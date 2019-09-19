/*
 * EmailRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "MasterServiceImpl.h"

using namespace std;

Status MasterServiceImpl::RequestRead(ServerContext* context, const ReadRequest* request, AddressDTO* response) {

	cerr << "Received request for user: " << request->username() << " file: " << request->uid() << endl;

	lock_guard<mutex> lock(beServerDataPtr->mtx);
	auto entry = metadataMap.find(request->username());

	if (entry != metadataMap.end()) {
		// Found the user
		unordered_map<string, MasterMetadata> fileMap = entry->second;

		auto fileEntry = fileMap.find(request->uid());
		if (fileEntry != fileMap.end()) {
			MasterMetadata fileData = fileEntry->second;
			// Found the file
			// Get the group of servers the file is stored in
			string address = getServerFromGroup(fileData.serverGroupId);
			response->set_address(address);
		}
		else {
			cerr << "File '" << request->uid() << "' for user '" << request->username() << "' does not exist" << endl;
			response->set_address("");
		}
	}
	else {
		// User not present
		cerr << "User '" << request->username() << "' does not exist" << endl;
		response->set_address("");

	}

	return Status::OK;
}

string MasterServiceImpl::getServerFromGroup(int groupId) {
	// Get the group of servers the file is stored in
	deque<ServerStatusDTO> servers = beServerDataPtr->serverGroupMap[groupId];

	vector<ServerStatusDTO> runningServers;
	for (ServerStatusDTO &server : servers) {
		if (server.status() == ServerStatus::RUNNING) {
			runningServers.push_back(server);
		}
	}

	string address;
	if (runningServers.size() > 0) {
		// Choose a server from the group
		std::uniform_int_distribution<> distr(0, runningServers.size() - 1); // define the range

		random_device rd; // obtain a random number from hardware
		mt19937 eng(rd()); // seed the generator
		int index = distr(eng);

		cerr << "Generated random index '" << index << "' between 0 and " << runningServers.size() - 1 << endl;

		address = runningServers[index].ipaddress() + ":" + to_string(runningServers[index].port());
	}

	return address;
}

int MasterServiceImpl::chooseServerGroup(const WriteRequest* request, MasterMetadata &existingFile) {
	auto entry = metadataMap.find(request->username());

	if (entry != metadataMap.end()) {
		// Found the user
		unordered_map<string, MasterMetadata> fileMap = entry->second;

		auto fileEntry = fileMap.find(request->uid());
		if (fileEntry != fileMap.end()) {
			MasterMetadata fileData = fileEntry->second;

			// File exists so write it to the same server group
			existingFile = fileData;
			return fileData.serverGroupId;
		}
	}

	// New file, choose the server with the least load
	int minLoad = -1;
	int serverGroup = -1;
	for (auto &entry : beServerDataPtr->loadMap) {
		cerr << "Server Group: " << entry.first << ", Load: " << entry.second << endl;
		if (minLoad < 0 || entry.second < minLoad) {
			minLoad = entry.second;
			serverGroup = entry.first;
		}
	}
	existingFile.filesize = 0;
	return serverGroup;
}

Status MasterServiceImpl::RequestWrite(ServerContext* context, const WriteRequest* request, WriteResponse* response) {

	unique_lock<mutex> lock(beServerDataPtr->mtx);
	MasterMetadata existingFile(request->username(), request->uid(), request->filetype(), 0, 0);
	int serverGroupId = chooseServerGroup(request, existingFile);
	deque<ServerStatusDTO> serverList = beServerDataPtr->serverGroupMap[serverGroupId];
	int seqNum = beServerDataPtr->seqNumMap[serverGroupId]++;
	cerr << "Sequence Number for server group " << serverGroupId << ": " << seqNum << endl;
	lock.unlock();

	int fileSize = request->value().size();
	MasterMetadata metadata(request->username(), request->uid(), request->filetype(), fileSize, serverGroupId);

	// Set primary server address
	string primaryServerAddress = serverList[0].ipaddress() + ":" + to_string(serverList[0].port());
	cerr << "Sending write request to " << primaryServerAddress << endl;
	WriteServiceClient writeClient(grpc::CreateChannel(primaryServerAddress, grpc::InsecureChannelCredentials()));

	vector<string> replicas;
	for (int i = 1; i < serverList.size(); i++) {
		replicas.push_back(serverList[i].ipaddress() + ":" + to_string(serverList[i].port()));
	}

	WriteCommandResponse writeResponse = writeClient.WriteCommand(request->username(), request->uid(), request->value(),
			request->type(), request->filetype(), replicas, seqNum);

	if (writeResponse.success()) {
		lock_guard<mutex> guard(beServerDataPtr->mtx);
		if (request->type() == WriteType::PUT) {
			cerr << "Writing file: (" << request->username() << ", " << request->uid() << ")" << endl;
			// If the file already exists, update load by difference between old file and new file
			beServerDataPtr->loadMap[serverGroupId] += fileSize - existingFile.filesize;
			// Write was a success. Add metadata to map
			auto entry = metadataMap.find(request->username());

			if (entry != metadataMap.end()) {
				// Found the user
				unordered_map<string, MasterMetadata>* fileMap = &(entry->second);
				// NOTE: insert will not overwrite, so first erase
				fileMap->erase(request->uid());
				fileMap->insert(pair<string, MasterMetadata>(request->uid(), metadata));
				cerr << "User: " << request->username() << " File Count: " << fileMap->size() << endl;
//				for (auto &fileentry : *fileMap) {
//					cerr << "filename: " << fileentry.first << endl;
//				}
			}
			else {
				// This is the first file for the user
				unordered_map<string, MasterMetadata> fileMap;
				fileMap.insert(pair<string, MasterMetadata>(request->uid(), metadata));
				metadataMap.insert(pair<string, unordered_map<string, MasterMetadata>>(request->username(), fileMap));
				beServerDataPtr->loadMap[serverGroupId]++;
				cerr << "User: " << request->username() << " File Count: " << fileMap.size() << endl;
//				for (auto &entry : fileMap) {
//					cerr << "filename: " << entry.first << endl;
//				}
			}
		}
		else if(request->type() == WriteType::DELETE){
			cerr << "Deleting file: (" << request->username() << ", " << request->uid() << ")" << endl;
			// WriteType::DELETE: Remove from master metadata
			beServerDataPtr->loadMap[serverGroupId] -= existingFile.filesize;
			// Write was a success. Add metadata to map
			auto entry = metadataMap.find(request->username());

			if (entry != metadataMap.end()) {
				// Found the user
				unordered_map<string, MasterMetadata>* fileMap = &(entry->second);
				fileMap->erase(request->uid());
				cerr << "User: " << request->username() << " File Count: " << fileMap->size() << endl;
//				for (auto &fileentry : *fileMap) {
//					cerr << "filename: " << fileentry.first << endl;
//				}
			}

		}


		response->set_success(true);
	}
	else {
		cerr << "Failed to write file" << endl;
		response->set_success(false);
	}


	return Status::OK;
}

Status MasterServiceImpl::RequestEmailList(ServerContext* context, const FileListRequest* request, ServerWriter<MasterFileDTO>* writer) {
	vector<MasterFileDTO> files;

	lock_guard<mutex> lock(beServerDataPtr->mtx);
	cerr << "Request for files for user: " << request->username() << endl;
	auto entry = metadataMap.find(request->username());

		if (entry != metadataMap.end()) {
			// Found the user
			unordered_map<string, MasterMetadata> fileMap = entry->second;

			for (auto& file : fileMap) {
				MasterMetadata metadata = file.second;

				if (metadata.filetype == request->filetype() || request->filetype() == FileType::ANY) {
					MasterFileDTO file;
					file.set_filetype(metadata.filetype);
					file.set_uid(metadata.filename);
					file.set_username(metadata.username);



					files.push_back(file);
				}
			}
		}


	for (MasterFileDTO file : files) {
		writer->Write(file);
	}

	return Status::OK;
}

Status MasterServiceImpl::RequestAddressesForFileList(ServerContext* context, const FileListRequest* request, ServerWriter<AddressDTO>* writer) {
	vector<AddressDTO> addresses;
	set<int> serverGroups;

	lock_guard<mutex> lock(beServerDataPtr->mtx);
	cerr << "Request for files for user: " << request->username() << endl;
	auto entry = metadataMap.find(request->username());

	if (entry != metadataMap.end()) {
		// Found the user
		unordered_map<string, MasterMetadata> fileMap = entry->second;

		for (auto& file : fileMap) {
			MasterMetadata metadata = file.second;

			if (metadata.filetype == request->filetype() || request->filetype() == FileType::ANY) {
				serverGroups.insert(metadata.serverGroupId);
			}
		}
	}

	cerr << "There are files on " << serverGroups.size() << " BE servers" << endl;
	for (int serverGroupId : serverGroups) {
		AddressDTO address;

		string addressStr = getServerFromGroup(serverGroupId);
		address.set_address(addressStr);

		addresses.push_back(address);
	}


	for (AddressDTO address : addresses) {
		writer->Write(address);
	}

	return Status::OK;
}

Status MasterServiceImpl::RequestAddressesForAllPrimaryServers(ServerContext* context, const FileListRequest* request, ServerWriter<AddressDTO>* writer) {
	vector<AddressDTO> addresses;

	lock_guard<mutex> lock(beServerDataPtr->mtx);
	for (auto &entry : beServerDataPtr->serverGroupMap) {
		if (entry.second.size() > 0) {
			AddressDTO primaryAddress;
			primaryAddress.set_address(entry.second[0].heartbeataddress());
			addresses.push_back(primaryAddress);
		}

	}
	cerr << "There are  " << beServerDataPtr->serverGroupMap.size() << " BE server groups" << endl;

	for (AddressDTO address : addresses) {
		writer->Write(address);
	}

	return Status::OK;
}

