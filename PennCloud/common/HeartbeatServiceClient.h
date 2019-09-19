/*
 * EmailRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef HEARTBEATSERVICECLIENT_H_
#define HEARTBEATSERVICECLIENT_H_

#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "HeartbeatService.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using heartbeatservice::HeartbeatService;
using heartbeatservice::ServerStatusDTO;
using heartbeatservice::HeartbeatRequest;
using heartbeatservice::ServerStatus;
using heartbeatservice::Empty;

using namespace std;

class HeartbeatServiceClient {
public:
	HeartbeatServiceClient(shared_ptr<Channel> channel) : stub_(HeartbeatService::NewStub(channel)) {

	}

	ServerStatusDTO heartbeat(string ipAddress, int port);

	bool killServer();

	unsigned int getTimeout() const {
		return timeout;
	}

	void setTimeout(unsigned int timeout = 30) {
		this->timeout = timeout;
	}

private:
	unique_ptr<HeartbeatService::Stub> stub_;
	unsigned int timeout = 30;
};


#endif /* HEARTBEATSERVICECLIENT_H_ */
