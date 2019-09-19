/*
 * UserRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef FE_SERVER_USERREPOSITORYCLIENT_H_
#define FE_SERVER_USERREPOSITORYCLIENT_H_

#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "../common/UserRepository.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using userrepository::UserRepository;
using userrepository::UserDTO;
using userrepository::GetUserRequest;
using userrepository::CreateUserResponse;

using namespace std;

class UserRepositoryClient {
public:
	UserRepositoryClient(shared_ptr<Channel> channel) : stub_(UserRepository::NewStub(channel)) {

	}

	/**
	 * Get the user data for the user with the specified username.
	 *
	 * @param username
	 * 		the user name of the user to retrieve
	 */
	UserDTO getUser(string username);

	/**
	 * Create a user.
	 *
	 * @param user
	 * 		the user data
	 */
	CreateUserResponse createUser(UserDTO user);

private:
	unique_ptr<UserRepository::Stub> stub_;
};


#endif /* FE_SERVER_USERREPOSITORYCLIENT_H_ */
