/*
 * EmailRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef SERVERTOSERVERCLIENT_H_
#define SERVERTOSERVERCLIENT_H_

#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "ServerToServer.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using server2server::ServerToServer;
using server2server::LogRequest;
using writeservice::WriteCommandRequest;

using namespace std;

class ServerToServerClient {
public:
	ServerToServerClient(shared_ptr<Channel> channel) : stub_(ServerToServer::NewStub(channel)) {

	}

	vector<WriteCommandRequest> AskForLog(int sequenceNum);

private:
	unique_ptr<ServerToServer::Stub> stub_;
};


#endif /* SERVERTOSERVERCLIENT_H_ */
