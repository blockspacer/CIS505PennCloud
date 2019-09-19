/*
 * AdminConsoleRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef BE_SERVER_ADMINCONSOLEREPOSITORYIMPL_H_
#define BE_SERVER_ADMINCONSOLEREPOSITORYIMPL_H_

#include <signal.h>

#include <iostream>
#include <memory>
#include <string>

#include "BigTable.h"
#include "../common/AdminConsoleRepository.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

using adminconsolerepository::AdminConsoleRepository;
using adminconsolerepository::GetRowsRequest;
using adminconsolerepository::GetUserRowRequest;
using adminconsolerepository::BigTableRow;
using adminconsolerepository::File;

class AdminConsoleRepositoryImpl final : public AdminConsoleRepository::Service {

public:
	explicit AdminConsoleRepositoryImpl(BigTable* bigTablePtr) {
		this->bigTablePtr = bigTablePtr;
	}

	Status GetRowForUser(ServerContext* context, const GetUserRowRequest* request, BigTableRow* response) override;

	Status GetRows(ServerContext* context, const GetRowsRequest* request, ServerWriter<BigTableRow>* writer) override;


private:
	BigTable* bigTablePtr;
};

#endif /* BE_SERVER_ADMINCONSOLEREPOSITORYIMPL_H_ */
