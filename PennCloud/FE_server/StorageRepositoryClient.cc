/*
 * StorageRespositoryClient.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */


#include "StorageRepositoryClient.h"
/**
 * generate uid for each file
 */
void computeDigest(const char *data, int dataLengthBytes, unsigned char *digestBuffer)
{
	/* The digest will be written to digestBuffer, which must be at least MD5_DIGEST_LENGTH bytes long */

	MD5_CTX c;
	MD5_Init(&c);
	MD5_Update(&c, data, dataLengthBytes);
	MD5_Final(digestBuffer, &c);
}


void printFileDTO(FileDTO& file){

	cerr << "username: " << file.owner() << endl;
	cerr << "filename: " << file.filename() << endl;
	cerr << "uid: " << file.uid() << endl;
	cerr << "type: " << file.type() << endl;
//	cerr << "data: " << endl << file.data() << endl;
	cerr << "size: " << file.size() << endl;
	cerr << "time: " << file.time() << endl;
}

FileDTO StorageRepositoryClient::getFile(string user, string fuid) {
	ClientContext context;

	GetFileRequest request;
	request.set_username(user);
	request.set_filename(fuid);

	FileDTO file;

	Status status = stub_->GetFile(&context, request, &file);
	if (!status.ok()) {
		// log
		cerr << "GetFile rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
	}
	else {
		cerr << "GetFile rpc succeeded" << endl;
		// printFileDTO(file);
	}

	return file;
}

vector<FileDTO> StorageRepositoryClient::getFileList(string user, string folder) {
	ClientContext context;

	GetFileRequest request;
	request.set_username(user);
	request.set_filename(folder);

	unique_ptr<ClientReader<FileDTO>> reader(stub_->GetFileList(&context, request));

	FileDTO file;
	vector<FileDTO> files;
	while (reader->Read(&file)) {
		files.push_back(file);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "GetFileList rpc failed." << endl;
	} else {
		cerr << "GetFileList rpc succeeded." << endl;
		cerr << "Get " << files.size() << " files" << endl;
		for (auto it = files.begin(); it != files.end(); it++){
			// printFileDTO(*it);
		}
	}

	return files;
}

vector<FileDTO> StorageRepositoryClient::getFilesByFolder(string user, string foldername) {
	ClientContext context;

	GetFileRequest request;
	request.set_username(user);
	request.set_filename(foldername);

	unique_ptr<ClientReader<FileDTO>> reader(stub_->GetFilesByFolder(&context, request));

	FileDTO file;
	vector<FileDTO> files;
	while (reader->Read(&file)) {
		files.push_back(file);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "getFilesByFolder rpc failed." << endl;
	} else {
		cerr << "getFilesByFolder rpc succeeded." << endl;
		cerr << "get "<<files.size()<<" files"<<endl;
		for (auto it = files.begin(); it != files.end(); it++){
			// printFileDTO(*it);
		}
	}

	return files;
}

AddFileResponse StorageRepositoryClient::addFile(FileDTO file) {
	ClientContext context;

	AddFileResponse response;

	// generate uid
	string filedata = file.data();
	unsigned char digestbuf[2*MD5_DIGEST_LENGTH+1];
	computeDigest(filedata.c_str(), filedata.length(), digestbuf);
	char BUFF[32];
	char MD5_id[2*MD5_DIGEST_LENGTH+1];
	memset(MD5_id, 0, sizeof(MD5_id));
	for (int j=0; j < MD5_DIGEST_LENGTH; j++){
		sprintf(BUFF, "%02x", digestbuf[j]);
		strcat(MD5_id, BUFF);
	} 
	// cerr << "convert uid from 128-b unsigned char:("<<digestbuf<<") to 32-digit string: ("<<MD5_id<<")"<<endl;
	file.set_uid(string(MD5_id));
	file.set_size(filedata.length());

	// set time string
	time_t curr = time(0);
	struct tm t = *localtime(&curr);
	char localTimeChar[30];
	memset(localTimeChar, 0, sizeof(localTimeChar));
	// strftime(localTimeChar, sizeof(localTimeChar), "%c %Z", &t);
	strftime(localTimeChar, sizeof(localTimeChar), "%c", &t);
	file.set_time(string(localTimeChar));
	printFileDTO(file);


	// cerr << "call StorageRepositoryImpl addFile begin"<<endl;
	Status status = stub_->AddFile(&context, file, &response);
	// cerr << "call StorageRepositoryImpl addFile" << file.filename() << " finishes" << endl;

	if (!status.ok()) {
		cerr << "AddFile rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
		response.set_success(false);
		response.set_message("AddFile RPC call failed");
	}

	return response;
}


AddFileResponse StorageRepositoryClient::deleteFile(string user, string filepath) {
	
	ClientContext context;
	GetFileRequest request;
	request.set_username(user);
	request.set_filename(filepath);
	AddFileResponse response;

	// cerr << "call StorageRepositoryImpl deleteFile begin" << endl;
	Status status = stub_->DeleteFile(&context, request, &response);

	if (!status.ok()) {
		cerr << "Delete rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
		response.set_success(false);
		response.set_message("DeleteFile RPC call failed");
	}

	return response;
}

/**
 * Add a new folder(empty) in the current directory (root directory is '/').
 * Treat empty folder as a special FileDTO with empty uid, and type "folder".
 * If the folder name has not been used, return true reponse.
 * Otherwise return false response with error message.
 */
AddFileResponse StorageRepositoryClient::addFolder(string user, string foldername) {

	ClientContext context;
	AddFileResponse response;

	FileDTO file;
	file.set_type("folder");
	file.set_owner(user);
	file.set_filename(foldername);
	// printFileDTO(file);

	Status status = stub_->AddFolder(&context, file, &response);
	if (!status.ok()) {
		cerr << "AddFile rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
		response.set_success(false);
		response.set_message("AddFile RPC call failed");
	}

	return response;
}


AddFileResponse StorageRepositoryClient::deleteFolder(string user, string foldername) {
	
	ClientContext context;
	GetFileRequest request;
	request.set_username(user);
	request.set_filename(foldername);
	AddFileResponse response;

	Status status = stub_->DeleteFolder(&context, request, &response);

	if (!status.ok()) {
		cerr << "Delete rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
		response.set_success(false);
		response.set_message("DeleteFile RPC call failed");
	}

	return response;
}


RenameFileResponse StorageRepositoryClient::renameFile(string user, string oldname, string newname) {
	ClientContext context;
	RenameFileRequest request;
	request.set_username(user);
	request.set_oldname(oldname);
	request.set_newname(newname);
	RenameFileResponse response;

	Status status = stub_->RenameFile(&context, request, &response);

	if (!status.ok()) {
		cerr << "renameFile rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
		response.set_success(false);
		response.set_message("renameFile RPC call failed");
	}

	return response;
}


RenameFileResponse StorageRepositoryClient::renameFolder(string user, string oldname, string newname) {
	ClientContext context;
	RenameFileRequest request;
	request.set_username(user);
	request.set_oldname(oldname);
	request.set_newname(newname);
	RenameFileResponse response;

	Status status = stub_->RenameFolder(&context, request, &response);

	if (!status.ok()) {
		cerr << "renameFolder rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
		response.set_success(false);
		response.set_message("renameFolder RPC call failed");
	}

	return response;
}

MoveFileResponse StorageRepositoryClient::moveFile(string user, string filename, string destfolder) {

	ClientContext context;
	MoveFileRequest request;
	request.set_username(user);
	request.set_filename(filename);
	request.set_foldername(destfolder);
	MoveFileResponse response;

	Status status = stub_->MoveFile(&context, request, &response);

	if (!status.ok()) {
		cerr << "moveFile rpc failed." << endl;
		cerr << "Status code: " << status.error_code() << endl;
		cerr << "Failure message: " << status.error_message() << endl;
		response.set_success(false);
		response.set_message("moveFile RPC call failed");
	}

	return response;
}
