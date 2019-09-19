/*
 * StorageRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef FE_SERVER_STORAGEREPOSITORYCLIENT_H_
#define FE_SERVER_STORAGEREPOSITORYCLIENT_H_

#include <ctime>
#include <time.h>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include <grpc++/grpc++.h>

#include "../common/StorageRepository.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using storagerepository::StorageRepository;
using storagerepository::FileDTO;
using storagerepository::GetFileRequest;
using storagerepository::AddFileResponse;
using storagerepository::RenameFileRequest;
using storagerepository::RenameFileResponse;
using storagerepository::MoveFileRequest;
using storagerepository::MoveFileResponse;

using namespace std;

class StorageRepositoryClient {
public:
	StorageRepositoryClient(shared_ptr<Channel> channel) : stub_(StorageRepository::NewStub(channel)) {

	}

	FileDTO getFile(string user, string fuid);

	vector<FileDTO> getFilesByFolder(string user, string foldername);

	vector<FileDTO> getFileList(string user, string foldername);

	AddFileResponse addFile(FileDTO file);

	AddFileResponse deleteFile(string user, string filepath);

	AddFileResponse addFolder(string user, string foldername);

	AddFileResponse deleteFolder(string user, string foldername);

	RenameFileResponse renameFile(string user, string oldname, string newname);

	MoveFileResponse moveFile(string user, string filename, string destfolder);

	RenameFileResponse renameFolder(string user, string oldname, string newname);


private:
	unique_ptr<StorageRepository::Stub> stub_;
};

void computeDigest(const char *data, int dataLengthBytes, unsigned char *digestBuffer);

#endif /* FE_SERVER_STORAGEREPOSITORYCLIENT_H_ */
