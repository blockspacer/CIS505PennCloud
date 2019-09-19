/*
 * EmailRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef WRITESERVICEIMPL_H_
#define WRITESERVICEIMPL_H_

#include <iostream>
#include <memory>
#include <string>

#include "../BE_server/StorageRepositoryImpl.h"
#include "../common/HeartbeatService.grpc.pb.h"
#include "WriteService.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>
#include "../BE_server/BigTable.h"
#include "../BE_server/Logger.h"

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using grpc::Server;
using grpc::ServerBuilder;

using writeservice::WriteService;
using writeservice::WriteType;
using writeservice::WriteCommandRequest;
using writeservice::WriteCommandResponse;
using writeservice::FileType;
using storagerepository::FileDTO;
using heartbeatservice::ServerStatus;

using namespace std;


struct WriteRequestComparator {
	bool operator() (const WriteCommandRequest& lhs, const WriteCommandRequest& rhs) {
		return lhs.seqnum() < rhs.seqnum();
	}
};

class WriteServiceImpl final : public WriteService::Service {

public:
	explicit WriteServiceImpl(BigTable* bigTable, Logger* logger, ServerStatus* state, int* seqNum, set<WriteCommandRequest, WriteRequestComparator> requestQueue) {
		this->bigTable = bigTable;
        this->seqNum = seqNum;
		this->requestQueue = requestQueue;
		this->logger = logger;

		if (*state == ServerStatus::RECOVERING) {
            cerr << "start to process requestQueue................." << endl;
			// clear the log, and add this new coming request to the hold-back queue first
			logger->clear();
			logger->print();

			// processing local log and remote log from primary first before you processing new coming requests
			while (!requestQueue.empty()) {
				auto firstIter = requestQueue.begin();
				WriteCommandRequest rq = *firstIter;
				cerr << "request.seqnum() = " << rq.seqnum() << endl;


				// call the corresponding function
				WriteCommandResponse response;
				WriteCommand(NULL, &rq, &response);
				requestQueue.erase(firstIter);
			}
            cerr << "end to process requestQueue................." << endl;
            bigTable->print();
            logger->print();

			*state = ServerStatus::RUNNING;
		}
	}

	Status WriteCommand(ServerContext* context, const WriteCommandRequest* request, WriteCommandResponse* response) override;

private:
	BigTable* bigTable;
	Logger* logger;
    int* seqNum;
	set<WriteCommandRequest, WriteRequestComparator> requestQueue;

	bool putHandler(const WriteCommandRequest* request);
	bool deleteHandler(const WriteCommandRequest* request);
	bool renameHandler(const WriteCommandRequest* request);
	bool renameFile(const WriteCommandRequest* request);
	bool putFile(const WriteCommandRequest* request);
	bool putFolder(const WriteCommandRequest* request);
	bool putPassword(const WriteCommandRequest* request);
	bool putEmail(const WriteCommandRequest* request);
	bool deleteFile(const WriteCommandRequest* request);
	bool deleteFolder(const WriteCommandRequest* request);
	bool deletePassword(const WriteCommandRequest* request);
	bool deleteEmail(const WriteCommandRequest* request);

	vector<flistline> renameHelper(string user, string oldname, string newname, vector<string>& unmatched, BigTable* bigTablePtr);
};



#endif /* SERVERTOMASTERIMPL_H_ */
