/*
 * EmailRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef MASTERSERVICEIMPL_H_
#define MASTERSERVICEIMPL_H_

#include <iostream>
#include <memory>
#include <string>
#include <random>
#include <deque>

#include "MasterService.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

#include "HeartbeatServiceClient.h"
#include "WriteServiceClient.h"

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using grpc::Server;
using grpc::ServerBuilder;

using masterservice::MasterService;
using masterservice::ReadRequest;
using masterservice::WriteRequest;
using masterservice::WriteResponse;
using masterservice::AddressDTO;
using masterservice::FileListRequest;
using masterservice::MasterFileDTO;

using namespace std;

typedef struct {
	volatile bool run = true;
	mutex mtx;
	map<int, deque<ServerStatusDTO>> serverGroupMap;
	map<int, int> seqNumMap;
	map<int, long> loadMap;
} BeServerData;

class MasterMetadata {
public:
	string username;
	string filename;
	FileType filetype;
	int filesize;
	int serverGroupId;

	MasterMetadata(string username, string filename, FileType filetype, int size, int serverGroupId) {
		this->username = username;
		this->filename = filename;
		this->filetype = filetype;
		this->filesize = size;
		this->serverGroupId = serverGroupId;
	}
};

class MasterServiceImpl final : public MasterService::Service {

public:
	explicit MasterServiceImpl(BeServerData* beServerDataPtr) {
		this->beServerDataPtr = beServerDataPtr;
	}
	Status RequestRead(ServerContext* context, const ReadRequest* request, AddressDTO* response) override;

	Status RequestWrite(ServerContext* context, const WriteRequest* request, WriteResponse* response) override;

	Status RequestEmailList(ServerContext* context, const FileListRequest* request, ServerWriter<MasterFileDTO>* writer) override;

	Status RequestAddressesForFileList(ServerContext* context, const FileListRequest* request, ServerWriter<AddressDTO>* writer) override;

	Status RequestAddressesForAllPrimaryServers(ServerContext* context, const FileListRequest* request, ServerWriter<AddressDTO>* writer) override;

private:
	int chooseServerGroup(const WriteRequest* request, MasterMetadata &existingFile);

	string getServerFromGroup(int groupId);
	// mapping from username to the files belong to this user
	unordered_map<string, unordered_map<string, MasterMetadata>> metadataMap;
	// The data about all the BE servers
	BeServerData* beServerDataPtr;
};





#endif /* SERVERTOMASTERIMPL_H_ */
