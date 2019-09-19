/*
 * EmailRepositoryClient.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef FE_SERVER_EMAILREPOSITORYCLIENT_H_
#define FE_SERVER_EMAILREPOSITORYCLIENT_H_

#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "../common/EmailRepository.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using emailrepository::EmailRepository;
using emailrepository::EmailDTO;
using emailrepository::GetEmailRequest;

using namespace std;

class EmailRepositoryClient {
public:
	EmailRepositoryClient(shared_ptr<Channel> channel) : stub_(EmailRepository::NewStub(channel)) {

	}

	EmailDTO getEmail(string user, string uid);

	vector<EmailDTO> getAllEmails(string user);

	vector<EmailDTO> getEmails(string user, int count, int offset);

	bool deleteEmail(string user, string uid);
	bool sendEmail(EmailDTO email_to_send);

private:
	unique_ptr<EmailRepository::Stub> stub_;
};


#endif /* FE_SERVER_EMAILREPOSITORYCLIENT_H_ */
