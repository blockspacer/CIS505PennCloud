/*
 * HeartbeatServiceClient.cc
 *
 *  Created on: Nov 18, 2017
 *      Author: cis505
 */


#include "WriteServiceClient.h"

WriteCommandResponse WriteServiceClient::WriteCommand(string user, string uid, string content, WriteType wtype, FileType ftype, vector<string> replicas, int seqNum) {
	ClientContext context;

	WriteCommandRequest request;
	// TODO: Fill out the request
	request.set_username(user);
	request.set_uid(uid);
	request.set_value(content);
	request.set_wtype(wtype);
	request.set_ftype(ftype);
	request.set_seqnum(seqNum);
	for (int i = 0; i < replicas.size(); i++) {
		request.add_replicas(replicas.at(i));
	}

	WriteCommandResponse response;

	Status status = stub_->WriteCommand(&context, request, &response);

	if (!status.ok()) {
		// TODO:
		response.set_success(false);
	}
	else {
		// TODO:
		response.set_success(true);
	}

	return response;
}

