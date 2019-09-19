/*
 * HeartbeatServiceClient.cc
 *
 *  Created on: Nov 18, 2017
 *      Author: cis505
 */


#include "ServerToServerClient.h"

vector<WriteCommandRequest> ServerToServerClient::AskForLog(int seqNum) {
	ClientContext context;

	LogRequest request;
	// TODO: set seqNum
	request.set_sequencenum(seqNum);

	unique_ptr<ClientReader<WriteCommandRequest>> reader(stub_->AskForLog(&context, request));

	WriteCommandRequest logEntry;
	vector<WriteCommandRequest> logEntries;
	while (reader->Read(&logEntry)) {
		logEntries.push_back(logEntry);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "AskForLog rpc failed." << endl;
	}

	return logEntries;
}

