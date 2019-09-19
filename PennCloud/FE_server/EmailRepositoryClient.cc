/*
 * EmailRespositoryClient.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "EmailRepositoryClient.h"
// HW2: test/common.cc

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

EmailDTO EmailRepositoryClient::getEmail(string user, string uid) {
	ClientContext context;

	GetEmailRequest request;
	request.set_username(user);
	request.set_uid(uid);

	EmailDTO email;

	cerr << "Calling GetEmail stub. " << endl;
	Status status = stub_->GetEmail(&context, request, &email);
	cerr << "Done calling GetEmail stub." << endl;
	if (!status.ok()) {
		// log
		cerr << "GetEmail rpc failed." << endl;
	}
	else {
		cerr << "GetEmail success." << endl;
	}

	return email;
}

vector<EmailDTO> EmailRepositoryClient::getAllEmails(string user) {
	ClientContext context;

	GetEmailRequest request;
	request.set_username(user);

	unique_ptr<ClientReader<EmailDTO>> reader(stub_->GetAllEmails(&context, request));

	EmailDTO email;
	vector<EmailDTO> emails;
	while (reader->Read(&email)) {
		emails.push_back(email);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "GetAllEmails rpc failed." << endl;
	}

	return emails;
}

vector<EmailDTO> EmailRepositoryClient::getEmails(string user, int count, int offset) {
	ClientContext context;

	GetEmailRequest request;
	request.set_username(user);
	request.set_count(count);
	request.set_offset(offset);

	unique_ptr<ClientReader<EmailDTO>> reader(stub_->GetNEmails(&context, request));

	EmailDTO email;
	vector<EmailDTO> emails;
	while (reader->Read(&email)) {
		emails.push_back(email);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "GetAllEmails rpc failed." << endl;
	}

	return emails;
}

bool EmailRepositoryClient::deleteEmail(string user, string uid) {
    ClientContext context;

	GetEmailRequest request;
	request.set_username(user);
	request.set_uid(uid);

	EmailDTO email;

	Status status = stub_->DeleteEmail(&context, request, &email);

	if(status.ok()){
		return true;
	}

	return  false;
}


bool EmailRepositoryClient::sendEmail(EmailDTO email_to_send){
	ClientContext context;

	EmailDTO email;
	
	Status status = stub_->SendEmail(&context, email_to_send, &email);
	if(status.ok()){
		return true;
	}
	return  false;
}


//AddFileResponse StorageRepositoryClient::addFile(FileDTO file) {
//	ClientContext context;
//
//	AddFileResponse response;
//
//	cerr << "call StorageRepositoryImpl begin"<<endl;
//	Status status = stub_->AddFile(&context, file, &response);
//	cerr << "call StorageRepositoryImpl finishes" << endl;
//
//	if (!status.ok()) {
//		cerr << "AddFile rpc failed." << endl;
//		cerr << "Status code: " << status.error_code() << endl;
//		cerr << "Failure message: " << status.error_message() << endl;
//		response.set_success(false);
//		response.set_message("AddFile RPC call failed");
//	}
//
//	return response;
//}
