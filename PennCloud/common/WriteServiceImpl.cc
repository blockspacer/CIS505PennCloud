/*
 * EmailRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "WriteServiceImpl.h"
#include "WriteServiceClient.h"

using namespace std;

bool WriteServiceImpl::putHandler(const WriteCommandRequest* request) {
	bool success = false;
	switch (request->ftype()) {
		case  FileType::EMAIL : {
			success = putEmail(request);
			break;
		}
		case FileType::FILE : {
			success = putFile(request);
			break;
		}
		case FileType::FOLDER : {
			success = putFolder(request);
			break;
		}
		case FileType::PASSWORD : {
			cerr << "Adding password to big table" << endl;
			success = putPassword(request);
			break;
		}
	}

	return success;
}

bool WriteServiceImpl::renameHandler(const WriteCommandRequest* request) {
	bool success = false;

	switch (request->ftype()) {
	case FileType::FILE: {
		success = renameFile(request);
		break;
	}

	}

	return success;
}

bool WriteServiceImpl::renameFile(const WriteCommandRequest* writeRequest) {
	bool success = false;

	RenameFileRequest request;
	request.ParseFromString(writeRequest->value());
	cerr<<"Received a request to rename file "<<request.oldname()<<endl;
	string user = request.username();
	// old file name format: /oldname
	// new file name format: /newname
	// both oldname and newname should contain full path to it
	string oldname = request.oldname();
	string newname = request.newname();

	std::vector<string> unmatched;
	std::vector<flistline> v = renameHelper(user, oldname, newname, unmatched, bigTable);

	if (v.size() == 0) {
		return false;
	}
	// keep unmatched entries unchanged
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// change file name to new name
	for (auto it = v.begin(); it != v.end(); it++) {
		struct flistline line = *it;
		string s = newname + "\t" + line.fuid + "\n";
		largestr.append(s);
	}

	if (largestr.length() == 0) {
		bigTable->dele(user, "filelist");
	} else {
		success = bigTable->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	return success;
}

bool WriteServiceImpl::deleteHandler(const WriteCommandRequest* request) {
	bool success = false;
	switch (request->ftype()) {
	case  FileType::EMAIL : {
		success = deleteEmail(request);
		break;
	}
	case FileType::FILE : {
		success = deleteFile(request);
		break;
	}
	case FileType::FOLDER : {
		success = deleteFolder(request);
		break;
	}
	case FileType::PASSWORD : {
		cerr << "Deleting password to big table" << endl;
		success = deletePassword(request);
		break;
	}
	}

	return success;
}

bool WriteServiceImpl::deleteEmail(const WriteCommandRequest* writeRequest) {
	return bigTable->dele(writeRequest->username(), writeRequest->uid());
}

bool WriteServiceImpl::deletePassword(const WriteCommandRequest* writeRequest) {
	return bigTable->dele(writeRequest->username(), writeRequest->uid());
}

bool WriteServiceImpl::deleteFile(const WriteCommandRequest* request) {
	cerr<<"Received a request to delete file"<<endl;
	string user = request->username();
	// filename format: /folder(s)/filename
	string filename = request->uid();
	cerr<<"user="<<user<<" ; "<<"filename="<<filename<<endl;

	// update filelist in table
	std::vector<string> unmatched;
	// use deleteFolderHelper here, not parseFileList [we may have multiple entries for a single large file->duplicate filename].
	std::vector<filemeta> v = StorageRepositoryImpl::deleteFolderHelper(user, filename, unmatched, bigTable);

	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}
	if (largestr.length() == 0) {
		bigTable->dele(user, "filelist");
	} else {
		bigTable->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	// delete file data from key-value storage
	for (auto it = v.begin(); it != v.end(); it++){
		struct filemeta m = *it;
		if (m.type == 1){
			cerr<<"find a matched file"<<endl;
			cerr<<"name="<<m.name<<"\t"<<"uid="<<m.fuid<<endl;
			bigTable->dele(user, m.fuid);
			cerr<<"delete "<<m.name<<" from key-value storage"<<endl;
		}
	}
	cerr<<"deleted "<<filename<<", "<<"update filelist for "<<user<<endl;
	cerr << "------------------begin the table ---------------------------" << endl;
	bigTable->print();
	cerr << "------------------end the table ---------------------------" << endl;

	return true;
}

bool WriteServiceImpl::deleteFolder(const WriteCommandRequest* request) {
	cerr<<"Received a request to delete folder"<<endl;
	string user = request->username();
	// foldername format: /foldername/
	string foldername = request->uid();

	std::vector<string> unmatched;
	std::vector<filemeta> v = StorageRepositoryImpl::deleteFolderHelper(user, foldername, unmatched, bigTable);
	// update filelist, or delete it if unmatched vector is empty
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}
	if (largestr.length() == 0) {
		bigTable->dele(user, "filelist");
	} else {
		bigTable->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	// delete file data from key-value storage
	for (auto it = v.begin(); it != v.end(); it++){
		struct filemeta m = *it;
		if (m.type == 1){
			cerr<<"name="<<m.name<<"\t"<<"uid="<<m.fuid<<" deleted "<<m.name<<" from key-value storage"<<endl;
			bigTable->dele(user, m.fuid);
		}
	}
	cerr << "see the table ---------------------------" << endl;
	bigTable->print();
	cerr << "see the table ---------------------------" << endl;

	return true;
}

bool WriteServiceImpl::putFile(const WriteCommandRequest* writeRequest) {

	FileDTO request;
	request.ParseFromString(writeRequest->value());
	cerr<<"Received a request to add file"<<endl;
	string user = request.owner();
	// filename format: /folder(s)/filename
	string filename = request.filename();
	cerr<<"user="<<user<<" ; "<<"filename="<<filename<<endl;


	std::vector<string> unmatched;
	std::vector<filemeta> v = StorageRepositoryImpl::parseFileList(user, filename, unmatched, bigTable);
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// delete old file data, not allow duplicate file names here
	if (v.size() > 0) {
		struct filemeta m = (filemeta)v.at(0);
		bigTable->dele(user, m.fuid);
	}
	string entry = filename + "\t" + request.uid() + "\n";
	largestr.append(entry);
	cerr<<"add new entry to filelist: ("<<filename<<"/r"<<request.uid()<<"/n"<<"), count="<<entry.length()<<endl;
	char buffer[50];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%ld", request.size());
	string filecontent = string(buffer) + "\n" + request.time() + "\n" + request.data();
	bigTable->put(user, request.uid(), request.type(), filecontent.length(), filecontent.c_str());
	cerr<<"put ("<<user<<","<<request.uid()<<") file in bigtable"<<endl;

	// update filelist in table
	bigTable->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	// cerr << "-----------------begin the table ---------------------------" << endl;
	// bigTablePtr->print();
	// cerr << "------------------end the table ---------------------------" << endl;

	return true;
}

bool WriteServiceImpl::putFolder(const WriteCommandRequest* writeRequest) {
	cerr<<"Received a request to add folder"<<endl;

	FileDTO request;
	request.ParseFromString(writeRequest->value());

	cerr<<"Received a request to add folder"<<endl;
	string user = request.owner();
	// format: /folder(s)/foldername
	string foldername = request.filename();

	std::vector<string> unmatched;
	std::vector<filemeta> v = StorageRepositoryImpl::parseFileList(user, foldername, unmatched, bigTable);
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// do nothing if folder name is duplicate
	if (v.size() > 0) {
		cerr << "Create folder failed, folder " + foldername + "already exists in current directory." << endl;;
		return false;
	}

	string entry = foldername + "\n";
	largestr.append(entry);
	cerr<<"add new entry to filelist: ("<<foldername<<"/n"<<"), count="<<entry.length()<<endl;

	// update filelist in table
	bigTable->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	cerr << "--------------------begin the table ---------------------------" << endl;
	bigTable->print();
	cerr << "--------------------end the table ---------------------------" << endl;

	return true;
}

bool WriteServiceImpl::putEmail(const WriteCommandRequest* request) {
	cerr<<"Put Email gets called! " << endl;

	return bigTable->put(request->username(), request->uid(), "email", request->value().size(), request->value().c_str());
}

bool WriteServiceImpl::putPassword(const WriteCommandRequest* request) {
	cerr<<"Username = " << request->username() << " password = \"" << request->value()<<"\""<< endl;

	bool result = bigTable->put(request->username(), request->uid(), "password", request->value().size(), request->value().c_str());

	cerr << "Finished password put." << endl;
	string usrPassword = bigTable->get(request->username(),"password");
	cerr<<"BE | CreateUser(): usrPassword is \""<< usrPassword<<"\""<< endl;

	return result;
}

Status WriteServiceImpl::WriteCommand(ServerContext* context, const WriteCommandRequest* request, WriteCommandResponse* response) {

	cerr << "Received a write command" << endl;
    *seqNum = request->seqnum();
    bool result = true;

	// 1. Loop over all the replicas, forward the write command request
	const ::google::protobuf::RepeatedPtrField<string> replicas = request->replicas();
	vector<string> tmp;

	for (int i = 0; i < replicas.size(); i++) {
		WriteServiceClient client(grpc::CreateChannel(replicas.Get(i), grpc::InsecureChannelCredentials()));
        WriteCommandResponse response = client.WriteCommand(request->username(), request->uid(), request->value(), request->wtype(), request->ftype(), tmp, request->seqnum());
	    if (!response.success()) {
            result = false;
        }
    }
	// 2. write to log
    if (request->replicas().size() > 0) {
        WriteCommandRequest rq;
        rq.set_username(request->username());
        rq.set_uid(request->uid());
        rq.set_value(request->value());
        rq.set_wtype(request->wtype());
        rq.set_ftype(request->ftype());
        rq.set_seqnum(request->seqnum());
        for (int i = 0; i < tmp.size(); i++) {
            rq.add_replicas(tmp.at(i));
        }

        string logEntry = rq.SerializeAsString();
        logger->append(logEntry);
    } else {
        string logEntry = request->SerializeAsString();
        logger->append(logEntry);
    }


	// 3. call the corresponding function
	switch (request->wtype()) {
		case WriteType::PUT : {
			result = putHandler(request);
			break;
		}
		case WriteType::DELETE : {
			result = deleteHandler(request);
			break;
		}
		case WriteType::RENAME: {
			result = renameHandler(request);
			break;
		}
	}
	cerr << "Finished processing write request" << endl;
    cerr << "delete disk" << endl;
//    bigTable->dele_disk();
    // print out debug messages
    bigTable->print();
    logger->print();
    if (result) {
        return Status::OK;
    } else {
        return Status::CANCELLED;
    }

}

/* Find meta information of a given file in filelist, return name and uid for each.
 * Duplicated name is allowed.
 * A large file may have multiple chunks in storage.
 */
vector<flistline> WriteServiceImpl::renameHelper(string user, string oldname, string newname, vector<string>& unmatched, BigTable* bigTablePtr) {

	std::vector<flistline> v;
	if (!bigTablePtr->exists(user, "filelist")) {
		return v;
	}
	string rawdata = bigTablePtr->get(user, "filelist");

	size_t len = oldname.length();
	size_t start = 0;
	size_t pos = rawdata.find("\n");
	string line = "";
	while(pos != std::string::npos){
		line = rawdata.substr(start, pos-start);
		struct flistline item;
		if (line.compare(0, newname.length(), newname) == 0) {
			vector<flistline> empty;
			return empty;
		}
		size_t found = line.find("\t");
		// lowest level is empty folder
		if (found == std::string::npos){
			if (line.compare(0, len, oldname) == 0){
				item.fuid = "";
				item.name = line.substr();
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		} else {
			// lowest level is file
			string firstentry = line.substr(0, found);
			if (firstentry.compare(0, len, oldname) == 0){
				item.name = firstentry.substr();
				item.fuid = line.substr(found+1);
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		}
		start = pos+1;
		pos = rawdata.find("\n", start);
	}

	return v;
}

