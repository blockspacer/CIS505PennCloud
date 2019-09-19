/*
 * EmailRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef SERVERTOMASTERIMPL_H_
#define SERVERTOMASTERIMPL_H_

#include <iostream>
#include <memory>
#include <string>

#include "ServerToMaster.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

#include "MasterServiceImpl.h"

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using grpc::Server;
using grpc::ServerBuilder;

using server2master::ServerToMaster;
using server2master::PrimaryRequest;
using masterservice::AddressDTO;

using namespace std;

class ServerToMasterImpl final : public ServerToMaster::Service {

public:
	explicit ServerToMasterImpl(BeServerData* beServerDataPtr) {
		this->beServerDataPtr = beServerDataPtr;
	}
	Status AskForPrimary(ServerContext* context, const PrimaryRequest* request, AddressDTO* response) override;

private:
	// The data about all the BE servers
	BeServerData* beServerDataPtr;
};





#endif /* SERVERTOMASTERIMPL_H_ */
