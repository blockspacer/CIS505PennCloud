/*
 * EmailRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef SERVERTOMASTERCLIENT_H_
#define SERVERTOMASTERCLIENT_H_

#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "ServerToMaster.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using server2master::ServerToMaster;
using server2master::PrimaryRequest;
using masterservice::AddressDTO;

using namespace std;

class ServerToMasterClient {
public:
	ServerToMasterClient(shared_ptr<Channel> channel) : stub_(ServerToMaster::NewStub(channel)) {

	}

	AddressDTO AskForPrimary(int groupId);

private:
	unique_ptr<ServerToMaster::Stub> stub_;
};


#endif /* SERVERTOMASTERCLIENT_H_ */
