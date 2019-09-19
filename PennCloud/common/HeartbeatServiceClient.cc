/*
 * HeartbeatServiceClient.cc
 *
 *  Created on: Nov 18, 2017
 *      Author: cis505
 */


#include "HeartbeatServiceClient.h"

ServerStatusDTO HeartbeatServiceClient::heartbeat(string ipAddress, int port) {
	ClientContext context;

	// Set a timeout for this call
	chrono::system_clock::time_point deadline = chrono::system_clock::now() + chrono::seconds(timeout);
	context.set_deadline(deadline);

	HeartbeatRequest request;

	ServerStatusDTO serverStatus;

	cerr << "Calling heartbeat stub." << endl;
	Status status = stub_->heartbeat(&context, request, &serverStatus);
	cerr << "Done calling heartbeat stub." << endl;
	if (!status.ok()) {
		// log
		cerr << "Heartbeat rpc failed for " << ipAddress << ":" << to_string(port) << endl;
		serverStatus.set_ipaddress(ipAddress);
		serverStatus.set_port(port);
		serverStatus.set_status(ServerStatus::UNKNOWN);
	}
	else {
		cerr << "Heartbeat success: " << ipAddress << ":" << to_string(port) << endl;
	}

	return serverStatus;
}

bool HeartbeatServiceClient::killServer() {
	ClientContext context;

	Empty request;

	Empty response;

	cerr << "Calling killServer stub. " << endl;
	Status status = stub_->killServer(&context, request, &response);
	cerr << "Done calling killServer stub." << endl;
	if (!status.ok()) {
		// log
		cerr << "killServer rpc failed." << endl;
		return false;
	}
	else {
		cerr << "killServer success." << endl;
		return true;
	}
}

