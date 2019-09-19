/*
 * EmailRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef WRITESERVICECLIENT_H_
#define WRITESERVICECLIENT_H_


#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "WriteService.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using writeservice::WriteService;
using writeservice::WriteType;
using writeservice::WriteCommandRequest;
using writeservice::WriteCommandResponse;
using writeservice::FileType;

using namespace std;

class WriteServiceClient {
public:
	WriteServiceClient(shared_ptr<Channel> channel) : stub_(WriteService::NewStub(channel)) {

	}

	WriteCommandResponse WriteCommand(string user, string uid, string content, WriteType wtype, FileType ftype, vector<string> replicas, int seqNum);

private:
	unique_ptr<WriteService::Stub> stub_;
};


#endif /* WRITESERVICECLIENT_H_ */
