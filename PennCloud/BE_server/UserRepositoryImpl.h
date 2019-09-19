/*
 * UserRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef BE_SERVER_USERREPOSITORYIMPL_H_
#define BE_SERVER_USERREPOSITORYIMPL_H_

#include <iostream>
#include <memory>
#include <string>

#include "BigTable.h"
#include "../common/UserRepository.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

using userrepository::UserRepository;
using userrepository::GetUserRequest;
using userrepository::UserDTO;
using userrepository::CreateUserResponse;

class UserRepositoryImpl final : public UserRepository::Service {

public:
	explicit UserRepositoryImpl(BigTable* bigTablePtr) {
		this->bigTablePtr = bigTablePtr;
	}
	Status GetUser(ServerContext* context, const GetUserRequest* request, UserDTO* response) override;

	Status CreateUser(ServerContext* context, const UserDTO* request, CreateUserResponse* response) override;

private:
	BigTable* bigTablePtr;
};





#endif /* BE_SERVER_USERREPOSITORYIMPL_H_ */
