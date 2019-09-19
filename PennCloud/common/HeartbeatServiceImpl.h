/*
 * EmailRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef HEARTBEATSERVICEIMPL_H_
#define HEARTBEATSERVICEIMPL_H_

#include <iostream>
#include <memory>
#include <string>

#include "HeartbeatService.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using grpc::Server;
using grpc::ServerBuilder;

using heartbeatservice::HeartbeatService;
using heartbeatservice::HeartbeatRequest;
using heartbeatservice::ServerStatusDTO;
using heartbeatservice::ServerStatus;
using heartbeatservice::Empty;

using namespace std;

class HeartbeatServiceImpl final : public HeartbeatService::Service {

public:
	explicit HeartbeatServiceImpl(string ipAddress, int port, ServerStatus *serverStatusPtr) {
		this->ipAddress = ipAddress;
		this->port = port;
		this->serverStatusPtr = serverStatusPtr;
	}
	Status heartbeat(ServerContext* context, const HeartbeatRequest* request, ServerStatusDTO* response) override;

	Status killServer(ServerContext* context, const Empty* request, Empty* response) override;

	unique_ptr<Server> runHeartbeatServer(string heartbeatServerAddress);

private:
	string ipAddress;
	int port;
	ServerStatus* serverStatusPtr;
};





#endif /* HEARTBEATSERVICEIMPL_H_ */
