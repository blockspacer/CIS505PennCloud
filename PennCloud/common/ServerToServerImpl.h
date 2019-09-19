/*
 * EmailRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef SERVERTOSERVERIMPL_H_
#define SERVERTOSERVERIMPL_H_

#include <iostream>
#include <memory>
#include <string>

#include "ServerToServer.grpc.pb.h"
#include "WriteService.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>
#include "../BE_server/Logger.h"

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using grpc::Server;
using grpc::ServerBuilder;

using server2server::ServerToServer;
using server2server::LogRequest;
using writeservice::WriteCommandRequest;

using namespace std;

class ServerToServerImpl final : public ServerToServer::Service {

public:
	explicit ServerToServerImpl(Logger* logger) {
		this->logger = logger;
	}
	Status AskForLog(ServerContext* context, const LogRequest* request, ServerWriter<WriteCommandRequest>* writer) override;

private:
	Logger* logger;
};





#endif /* SERVERTOSERVERIMPL_H_ */
