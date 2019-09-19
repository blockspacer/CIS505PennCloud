/*
 * EmailRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef MASTERSERVICECLIENT_H_
#define MASTERSERVICECLIENT_H_

#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "MasterService.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using masterservice::MasterService;
using masterservice::ReadRequest;
using masterservice::WriteRequest;
using masterservice::WriteResponse;
using masterservice::AddressDTO;
using masterservice::MasterFileDTO;
using masterservice::FileListRequest;
using writeservice::WriteType;
using writeservice::FileType;

using namespace std;

class MasterServiceClient {
public:
	MasterServiceClient(shared_ptr<Channel> channel) : stub_(MasterService::NewStub(channel)) {

	}

	AddressDTO RequestRead(string user, string filename);


	/**
	 * Request a write operation
	 */
	WriteResponse RequestWrite(string user, string filename, string content, WriteType wType, FileType fType);

	vector<MasterFileDTO> GetEmails(string user, FileType fileType);

	vector<AddressDTO> RequestAddressesForFileList(string user, FileType fileType);

	vector<AddressDTO> RequestAddressesForAllPrimaryServers();

private:
	unique_ptr<MasterService::Stub> stub_;
};


#endif /* MASTERSERVICECLIENT_H_ */
