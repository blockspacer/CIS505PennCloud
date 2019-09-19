/*
 * EmailRepositoryImpl.h
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#ifndef BE_SERVER_EMAILREPOSITORYIMPL_H_
#define BE_SERVER_EMAILREPOSITORYIMPL_H_

#include <iostream>
#include <memory>
#include <string>

#include "BigTable.h"
#include "../common/EmailRepository.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <grpc++/server_context.h>

using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

using emailrepository::EmailRepository;
using emailrepository::EmailDTO;
using emailrepository::GetEmailRequest;

class EmailRepositoryImpl final : public EmailRepository::Service {

public:
	explicit EmailRepositoryImpl(BigTable* bigTablePtr) {
		this->bigTablePtr = bigTablePtr;
	}
	Status GetEmail(ServerContext* context, const GetEmailRequest* request, EmailDTO* response) override;

	Status GetAllEmails(ServerContext* context, const GetEmailRequest* request, ServerWriter<EmailDTO>* writer) override;

	Status GetNEmails(ServerContext* context, const GetEmailRequest* request, ServerWriter<EmailDTO>* writer) override;

	Status DeleteEmail(ServerContext* context, const GetEmailRequest* request, EmailDTO* response) override;

	Status SendEmail(ServerContext* context, const EmailDTO* email_to_send, EmailDTO* response) override;


private:
	BigTable* bigTablePtr;
};

#define MAX_LEN_EACH_LINE (1000)

// -----------------------
// from HW2
#include <openssl/md5.h>
#include<fstream>
//----------------
#define MAX_FILENAME (100)
#define MAX_LEN_CMD (1024)
#define MAX_LEN_MESS (MAX_LEN_CMD*MAX_LEN_CMD)


struct Message{
	vector<string>content;
	bool deleted;
	int Len; //=0 no initializer
    char First_Line[MAX_LEN_CMD];
    char MD5_id_mess[2*MD5_DIGEST_LENGTH+1];
}; // must have this ;


int ImportMBOX(BigTable * table_pt,const string &userMBOX_addr, const string &username);
void computeDigest(char *data, int dataLengthBytes, unsigned char *digestBuffer);
#endif /* BE_SERVER_EMAILREPOSITORYIMPL_H_ */
