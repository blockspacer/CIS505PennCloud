/*
 * AdminConsoleRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef ADMINCONSOLEREPOSITORYCLIENT_H_
#define ADMINCONSOLEREPOSITORYCLIENT_H_

#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "../common/json.hpp"
#include "../common/AdminConsoleRepository.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using adminconsolerepository::AdminConsoleRepository;
using adminconsolerepository::GetRowsRequest;
using adminconsolerepository::GetUserRowRequest;
using adminconsolerepository::BigTableRow;
using adminconsolerepository::File;

using json = nlohmann::json;

using namespace std;

class AdminConsoleRepositoryClient {
public:
	AdminConsoleRepositoryClient(shared_ptr<Channel> channel) : stub_(AdminConsoleRepository::NewStub(channel)) {

	}

	BigTableRow getRow(string user);

	vector<BigTableRow> getAllRows();

	vector<BigTableRow> getRows(int count, int offset);

	string bigTableRowToJson(const BigTableRow & row);

	string bigTableRowsToJson(const vector<BigTableRow> &rows);

private:
	unique_ptr<AdminConsoleRepository::Stub> stub_;
};


#endif /* ADMINCONSOLEREPOSITORYCLIENT_H_ */
