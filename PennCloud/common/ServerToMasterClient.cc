/*
 * HeartbeatServiceClient.cc
 *
 *  Created on: Nov 18, 2017
 *      Author: cis505
 */


#include "ServerToMasterClient.h"

AddressDTO ServerToMasterClient::AskForPrimary(int groupId) {
	ClientContext context;

	PrimaryRequest request;
	AddressDTO address;

	Status status = stub_->AskForPrimary(&context, request, &address);

	if (!status.ok()) {
		// TODO:
	}
	else {
		// TODO:

	}

	return address;
}

