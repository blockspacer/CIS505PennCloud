/*
 * EmailRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "HeartbeatServiceImpl.h"

using namespace std;

Status HeartbeatServiceImpl::heartbeat(ServerContext* context, const HeartbeatRequest* request, ServerStatusDTO* response) {

	cerr << "Received a heartbeat request. In recovery: " << *serverStatusPtr << endl;
	response->set_ipaddress(ipAddress);
	response->set_port(port);
	response->set_status(*serverStatusPtr);

	return Status::OK;
}

Status HeartbeatServiceImpl::killServer(ServerContext* context, const Empty* request, Empty* response) {

	cerr << "Received kill request" << endl;
	raise(SIGINT);

	return Status::OK;
}

unique_ptr<Server> HeartbeatServiceImpl::runHeartbeatServer(string heartbeatServerAddress) {
	ServerBuilder builder;
	builder.AddListeningPort(heartbeatServerAddress, grpc::InsecureServerCredentials());
	builder.RegisterService(this);


	unique_ptr<Server> server(builder.BuildAndStart());
	cout << "Heartbeat server listening on " << heartbeatServerAddress << endl;

	return server;
}

