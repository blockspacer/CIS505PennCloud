/*
 * EmailRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "ServerToServerImpl.h"

using namespace std;

Status ServerToServerImpl::AskForLog(ServerContext* context, const LogRequest* request, ServerWriter<WriteCommandRequest>* writer) {

	int seqNum = request->sequencenum();

	string entry;
	string entrySize;
	vector<WriteCommandRequest> logEntries = logger->get();

	cerr << "End to read logs from primary. Sending " << logEntries.size() << " log entries" << endl;
	for (WriteCommandRequest logEntry : logEntries) {
		writer->Write(logEntry);
	}

	return Status::OK;
}

