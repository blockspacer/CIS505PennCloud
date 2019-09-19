/*
 * UserRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "UserRepositoryImpl.h"

using namespace std;

//Status UserRepositoryImpl::GetUser(ServerContext* context, const GetUserRequest* request, UserDTO* user) {
//
//	cerr << "Received a request for user" << endl;
//	// TODO: This is where we would retrieve the user data from the key-store (BigTable)
//	// using the data from in the request to find the user
//	user->set_username("user1");
//	user->set_password("password1");
//	user->set_nickname("nickname1");
//
//	cerr << "Returning the following user:" << endl;
//	cerr << user->SerializeAsString() << endl;
//
//	return Status::OK;
//}
Status UserRepositoryImpl::GetUser(ServerContext* context, const GetUserRequest* request, UserDTO* user) {

	cerr << "Received a request for user" << endl;
	// TODO: This is where we would retrieve the user data from the key-store (BigTable)
	// using the data from in the request to find the user
    // --- Below is Modified by Yuding Nov 21 ---------

    string usrPassword = bigTablePtr->get(request->username(),"password");
//    cerr<<"BE usrPassword is "<< usrPassword<<endl;
//    string usrNickname = bigTablePtr->get(request->username(),"nickname");
    if(usrPassword == ""){
        // the user don't exist
        user->set_username("notfound");
    } else {
        user->set_username(request->username());
        user->set_password(usrPassword);
//        user->set_nickname(usrNickname);
    }

    cerr<< "BE | GetUser(): Password is " << user->password() << " user name is "<<user->username()<<endl;
	cerr << "Returning the following user:" << endl;
	cerr << user->SerializeAsString() << endl;

	return Status::OK;
}

Status UserRepositoryImpl::CreateUser(ServerContext* context, const UserDTO* request, CreateUserResponse* response) {

	// TODO: Add the user to the key-value store
    // Nov 21, modified by Yuding
    cerr<<"Username = " << request->username() << " password = \"" << request->password()<<"\""<< endl;

    bigTablePtr->put(request->username(), "password","password", request->password().size(), request->password().c_str());
//    bigTablePtr->print();
	response->set_success(true);
	response->set_message("Successfully added user " + request->username());

    string usrPassword = bigTablePtr->get(request->username(),"password");
    cerr<<"BE | CreateUser(): usrPassword is \""<< usrPassword<<"\""<< endl;

    bigTablePtr->print();

	return Status::OK;
}


