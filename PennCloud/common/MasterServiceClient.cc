/*
 * HeartbeatServiceClient.cc
 *
 *  Created on: Nov 18, 2017
 *      Author: cis505
 */


#include "MasterServiceClient.h"

AddressDTO MasterServiceClient::RequestRead(string user, string filename) {
	ClientContext context;

	ReadRequest request;
	request.set_username(user);
	request.set_uid(filename);
	AddressDTO address;

	Status status = stub_->RequestRead(&context, request, &address);

	if (status.ok() && address.address() != "") {
		cerr << "MasterServiceClient::RequestRead completed successfully" << endl;
	}
	else {
		cerr << "MasterServiceClient::RequestRead failed" << endl;
	}

	return address;

}


/**
 * Request a write operation
 */
WriteResponse MasterServiceClient::RequestWrite(string user, string filename, string content, WriteType wType, FileType fType) {
	ClientContext context;

	WriteRequest request;
	request.set_username(user);
	request.set_uid(filename);
	request.set_value(content);
	request.set_type(wType);
	request.set_filetype(fType);
	WriteResponse response;

	Status status = stub_->RequestWrite(&context, request, &response);

	if (status.ok() && response.success()) {
		cerr << "MasterServiceClient::RequestWrite completed successfully" << endl;
	}
	else {
		cerr << "MasterServiceClient::RequestWrite failed" << endl;
	}

	return response;

}

vector<MasterFileDTO> MasterServiceClient::GetEmails(string user, FileType fileType) {
	ClientContext context;

	FileListRequest request;
	request.set_username(user);
	request.set_filetype(fileType);

	unique_ptr<ClientReader<MasterFileDTO>> reader(stub_->RequestEmailList(&context, request));

	MasterFileDTO file;
	vector<MasterFileDTO> files;
	while (reader->Read(&file)) {
		files.push_back(file);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "GetEmailList rpc failed." << endl;
	}

	return files;

}

vector<AddressDTO> MasterServiceClient::RequestAddressesForFileList(string user, FileType fileType) {
	ClientContext context;

	FileListRequest request;
	request.set_username(user);
	request.set_filetype(fileType);

	unique_ptr<ClientReader<AddressDTO>> reader(stub_->RequestAddressesForFileList(&context, request));

	AddressDTO address;
	vector<AddressDTO> addresses;
	while (reader->Read(&address)) {
		addresses.push_back(address);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "RequestAddressesForFileList rpc failed." << endl;
	}

	return addresses;

}

vector<AddressDTO> MasterServiceClient::RequestAddressesForAllPrimaryServers() {
	ClientContext context;

	FileListRequest request;

	unique_ptr<ClientReader<AddressDTO>> reader(stub_->RequestAddressesForAllPrimaryServers(&context, request));

	AddressDTO address;
	vector<AddressDTO> addresses;
	while (reader->Read(&address)) {
		addresses.push_back(address);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "RequestAddressesForFileList rpc failed." << endl;
	}

	return addresses;
}

