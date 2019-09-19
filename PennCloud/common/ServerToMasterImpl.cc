/*
 * EmailRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "ServerToMasterImpl.h"

using namespace std;

Status ServerToMasterImpl::AskForPrimary(ServerContext* context, const PrimaryRequest* request, AddressDTO* response) {

	cerr << "Received a primary request for Group " << request->groupnum() << endl;

	lock_guard<mutex> guard(beServerDataPtr->mtx);

	deque<ServerStatusDTO> serverGroup = beServerDataPtr->serverGroupMap[request->groupnum()];
	response->set_address(serverGroup[0].ipaddress() + ":" + to_string(serverGroup[0].port()));

	return Status::OK;
}

