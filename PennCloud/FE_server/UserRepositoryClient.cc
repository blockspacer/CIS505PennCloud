/*
 * UserRespositoryClient.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */


#include "UserRepositoryClient.h"

UserDTO UserRepositoryClient::getUser(string username) {
	ClientContext context;

	GetUserRequest request;
	request.set_username(username);

	UserDTO user;

	cerr << "Calling GetUser stub. " << endl;
	Status status = stub_->GetUser(&context, request, &user);
	cerr << "Done calling GetUser stub." << endl;
	if (!status.ok()) {
		// log
		cerr << "GetUser rpc failed." << endl;
	}
	else {
		cerr << "GetUser success." << endl;
	}

	return user;
}

CreateUserResponse UserRepositoryClient::createUser(UserDTO user) {
	ClientContext context;
	CreateUserResponse response;

	Status status = stub_->CreateUser(&context, user, &response);

	if (!status.ok()) {
		// log
		cerr << "GetUser rpc failed." << endl;
	}
	else {
		cerr << "GetUser rpc responded." << endl;
		if (response.success()) {
			cerr << "Successfully added user " << user.username() << endl;
		}
		else {
			cerr << "Failed to add user " << user.username() << ": " << response.message() << endl;
		}
	}

	return response;
}

