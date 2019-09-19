/*
 * StorageRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef BE_SERVER_STORAGEREPOSITORYIMPL_H_
#define BE_SERVER_STORAGEREPOSITORYIMPL_H_

#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include "BigTable.h"
#include "../common/StorageRepository.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

using storagerepository::StorageRepository;
using storagerepository::FileDTO;
using storagerepository::GetFileRequest;
using storagerepository::AddFileResponse;
using storagerepository::RenameFileRequest;
using storagerepository::RenameFileResponse;
using storagerepository::MoveFileRequest;
using storagerepository::MoveFileResponse;

struct filemeta{
	int type;    // 1 for file, 2 for folder
	int size;
	string name;
	string path;
	string fuid;
	string time;
};

class StorageRepositoryImpl final : public StorageRepository::Service {

public:
	explicit StorageRepositoryImpl(BigTable* bigTablePtr) {
		this->bigTablePtr = bigTablePtr;
	}
	static vector<filemeta> parseFileList(string user, string folder, vector<string>& unmatched, BigTable* bigTablePtr);
	static vector<filemeta> deleteFolderHelper(string user, string folder, vector<string>& unmatched, BigTable* bigTablePtr);
	/* Given username and file identifier(fuid), download one FileDTO */
	Status GetFile(ServerContext* context, const GetFileRequest* request, FileDTO* response) override;

	/* Given username and folder identifier(folder name), download a list of FileDTO */
	Status GetFilesByFolder(ServerContext* context, const GetFileRequest* request, ServerWriter<FileDTO>* writer) override;

	/* Given folder name, return the list of file names under that folder */
	Status GetFileList(ServerContext* context, const GetFileRequest* request, ServerWriter<FileDTO>* writer) override;

	/* Upload file to key-value storage */
	Status AddFile(ServerContext* context, const FileDTO* request, AddFileResponse* response) override;

	/* Delete file from key-value storage */
	Status DeleteFile(ServerContext* context, const GetFileRequest* request, AddFileResponse* response) override;

	/* Add a new folder in filelist */
	Status AddFolder(ServerContext* context, const FileDTO* request, AddFileResponse* response) override;

	/* Delete all files and subfolders in the given folder from key-value storage */
	Status DeleteFolder(ServerContext* context, const GetFileRequest* request, AddFileResponse* response) override;

	/* Rename the file by updating the filelist in key-value storage */
	Status RenameFile(ServerContext* context, const RenameFileRequest* request, RenameFileResponse* response) override;

	/* Move the file to another folder, update the filelist in key-value storage */
	Status MoveFile(ServerContext* context, const MoveFileRequest* request, MoveFileResponse* response) override;

	/* Rename the folder by updating the filelist in key-value storage */
	Status RenameFolder(ServerContext* context, const RenameFileRequest* request, RenameFileResponse* response) override;

private:
	BigTable* bigTablePtr;
};

struct flistline{
	string name;
	string fuid;
};

struct chunkmeta{
	int sequence; // sequence num of chunk file, start from 1
	string fuid;  // uid of the chunk file, used to retrieve chunk file data
};

struct compChunk {
	bool operator()(const chunkmeta& a, const chunkmeta& b) const {
		return a.sequence < b.sequence;
	}
};



#endif /* BE_SERVER_STORAGEREPOSITORYIMPL_H_ */
