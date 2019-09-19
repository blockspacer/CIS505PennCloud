/*
 * EmailRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "EmailRepositoryImpl.h"
//----------------
// file lock
// this should be 
#include <sys/file.h>
#include <fcntl.h>
#include <stdio.h>
#define LEN_MAX_CMD (1000)

using namespace std;

Status EmailRepositoryImpl::GetEmail(ServerContext* context, const GetEmailRequest* request, EmailDTO* email) {

	string email_Str = bigTablePtr->get(request->username(), request->uid());
	
	if (email_Str.size() > 0) {
		EmailDTO tmpEmail;
		tmpEmail.ParseFromString(email_Str);

		cerr << "Received a request for email" << endl;
		// TODO: This is where we would retrieve the email from the key-store (BigTable)
		// using the data from in the request to find the email
		email->set_uid(tmpEmail.uid());
		email->set_sender(tmpEmail.sender());
		email->set_date(tmpEmail.date());
		email->set_message(tmpEmail.message());
        email->set_subject(tmpEmail.subject());

		cerr << "Returning the following email:" << endl;
		cerr << email->SerializeAsString() << endl;
	}
	else { // I need to adapt to this error information
		cerr << "Could not find email for user: " << request->username() << " , uid: " << request->uid() << endl;
	}

	return Status::OK;
}

Status EmailRepositoryImpl::GetAllEmails(ServerContext* context, const GetEmailRequest* request,
		ServerWriter<EmailDTO>* writer) {

	cerr << "Request for all emails for user: " << request->username() << endl;
	unordered_map<string, Metadata> fileMap = bigTablePtr->get_user_files(request->username());

	vector<EmailDTO> emails;
	for (auto& file : fileMap) {
	    Metadata metadata = file.second;

	    if (metadata.filetype == "email") {
	    	string emailStr = bigTablePtr->get(request->username(), metadata.filename);
	    	EmailDTO email;
	    	email.ParseFromString(emailStr);
	    	emails.push_back(email);
	    }
	}

	for (EmailDTO email : emails) {
		writer->Write(email);
	}

	return Status::OK;
}

Status EmailRepositoryImpl::GetNEmails(ServerContext* context, const GetEmailRequest* request,
		ServerWriter<EmailDTO>* writer) {

	// TODO: Fill in with actual retrieval from key-value store

	EmailDTO email1;
	email1.set_uid("uid1");
	email1.set_sender("CIS505");
	email1.set_date("11/11/2017");
	email1.set_message("This is message 1");

	EmailDTO email2;
	email2.set_uid("uid2");
	email2.set_sender("CIS505");
	email2.set_date("11/10/2017");
	email2.set_message("This is message 2");

	writer->Write(email1);
	writer->Write(email2);

	return Status::OK;
}



Status  EmailRepositoryImpl::DeleteEmail(ServerContext* context, const GetEmailRequest* request, EmailDTO* response) {
    //TODO---
    cerr<<"DeleteEmail gets called! "<<endl;
    cerr<<request->username()<< "    "<< request->uid()<<endl;
    if(bigTablePtr->dele(request->username(), request->uid())){
        return Status::OK;
    }

    return Status::CANCELLED;

}




Status  EmailRepositoryImpl::SendEmail(ServerContext* context, const EmailDTO* email_to_send, EmailDTO* response) {
    //TODO---
    cerr<<"SendEmail gets called! "<<endl;
    //--------------------------
    // write first line -- it is required and it related with uniqueness of MD5 ID POP3
    char RequiredLine[LEN_MAX_CMD];
    memset(RequiredLine,0,sizeof(RequiredLine));
    sprintf(RequiredLine,"From <%s> ", email_to_send->sender().c_str());
    time_t CurrT = time(0);
    string timestamp = ctime(&CurrT); // end with \n\0
    strcat(RequiredLine,timestamp.c_str());
    RequiredLine[strlen(RequiredLine)-1] = '\r';
    RequiredLine[strlen(RequiredLine)] = '\n';
    
    char MD5_id_mess[2*MD5_DIGEST_LENGTH+1];
    memset(MD5_id_mess,0,sizeof(MD5_id_mess));
                    
    unsigned char digest[2*MD5_DIGEST_LENGTH+1];
    computeDigest(RequiredLine,strlen(RequiredLine),digest);
    // PIAZZA @252: You should convert MD5 hash to a string for UIDL in whatever way you like. 
    // A hex string might be the easiest one and satisfies the range requirements.
    //https://stackoverflow.com/questions/5661101/how-to-convert-an-unsigned-character-array-into-a-hexadecimal-string-in-c
    char BUFF[32];
    for (int j=0; j < MD5_DIGEST_LENGTH; j++){
        sprintf(BUFF, "%02x", digest[j]);// *2 == cannot connect to Thunderbird
        strcat(MD5_id_mess,BUFF);
    }

    EmailDTO email_to_send2;
    email_to_send2.set_uid(MD5_id_mess);
    // memcpy(&email_to_send2,email_to_send, sizeof(email_to_send));
    email_to_send2.set_subject(email_to_send->subject());
    email_to_send2.set_sender(email_to_send->sender());
    email_to_send2.set_receiver(email_to_send->receiver());
    email_to_send2.set_date(email_to_send->date());
    email_to_send2.set_message(email_to_send->message());
    
    //--------------------------
    cerr<<email_to_send2.sender()<< "    "<< email_to_send2.uid()<<endl;
    string serializedEmail = email_to_send2.SerializeAsString();


    if(bigTablePtr->put(email_to_send2.receiver(), email_to_send2.uid(),"email",serializedEmail.size(), serializedEmail.c_str()  )
        ){
        return Status::OK;
    }

    return Status::CANCELLED;

}


void computeDigest(char *data, int dataLengthBytes, unsigned char *digestBuffer)
{
  /* The digest will be written to digestBuffer, which must be at least MD5_DIGEST_LENGTH bytes long */
  MD5_CTX c;
  MD5_Init(&c);
  MD5_Update(&c, data, dataLengthBytes);
  MD5_Final(digestBuffer, &c);
}

int ImportMBOX(BigTable * table_pt,const string &OpenName, const string &username){
    vector<Message> MailDrop;           
    FILE *fd_pt_to_lock = fopen(OpenName.c_str(), "rb");
    if (NULL == fd_pt_to_lock)
    {string Err("-ERR opening file:");Err =Err+ string(OpenName)+"\r\n";
    }else{
        if(0!= flock(fileno(fd_pt_to_lock), LOCK_EX | LOCK_NB) )
        { char ErrAlreadyLocked[] ="-Err this user already locked\r\n";
        } else{
            char commd[MAX_LEN_CMD];
            char buff[MAX_LEN_CMD],  mess2[2*MAX_LEN_CMD]; //message[2*MAX_LEN_CMD],
            memset(commd, 0, sizeof(commd) );
            //char * LastPt; // LastPt is the pointer to an end of buff, in order to give COMMD correct command
            //memset(buff, 0, sizeof(buff) );
            int count = 0;
            Message NewMess;
            NewMess.content.clear();NewMess.Len=0;
            memset(NewMess.First_Line,0, sizeof(NewMess.First_Line));
            memset(NewMess.MD5_id_mess,0,sizeof(NewMess.MD5_id_mess)); 
            NewMess.deleted = false;
            while(NULL != fgets(&commd[strlen(commd)],2,fd_pt_to_lock))
            {// fgets will read one char and add \0 at the end -- in all there is only one char
                if (NULL!=strstr(commd, "\r\n") and strlen(commd) <= MAX_LEN_CMD-10)
                { 
                    if (NULL!= strstr(commd,"From <"))
                    {
                    	//printf("%s###########dddddddddddddddddd########################%s\n",commd,NewMess.MD5_id_mess);
                        //if (DEBUG_flag and strlen(commd)>0) cout << "lyz: here is OK###"<<commd<<"#"<<count<<"###\r\n";
                        NewMess.Len = count; 
                        //if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << commd << "####" << NewMess.Len << '\n';
                        MailDrop.push_back(NewMess); 
                        // initialize
                        NewMess.content.clear();NewMess.Len=0;count = 0;
                        memset(NewMess.First_Line,0, sizeof(NewMess.First_Line));
                        // this commd is the indicator of next message
                        strcpy(NewMess.First_Line,commd);
                        memset(NewMess.MD5_id_mess,0,sizeof(NewMess.MD5_id_mess));
                        
                        unsigned char digest[2*MD5_DIGEST_LENGTH+1];
                        computeDigest(commd,strlen(commd),digest);
                        // PIAZZA @252: You should convert MD5 hash to a string for UIDL in whatever way you like. 
                        // A hex string might be the easiest one and satisfies the range requirements.

                        //https://stackoverflow.com/questions/5661101/how-to-convert-an-unsigned-character-array-into-a-hexadecimal-string-in-c
                        char BUFF[32];
                        for (int j=0; j < MD5_DIGEST_LENGTH; j++){
                            sprintf(BUFF, "%02x", digest[j]);// *2 == cannot connect to Thunderbird
                            strcat(NewMess.MD5_id_mess,BUFF);
                        }
               
                        NewMess.deleted = false;
                    }else{
                        NewMess.content.push_back(string(commd)); // using string rather than char * is always safer
                        count +=strlen(commd);
                    }
                    memset(commd, 0, sizeof(commd) );
                }
            }
            NewMess.Len = count; 
            MailDrop.push_back(NewMess); // Last Message to put into the MailDrop

            //Erase first message (empty)
            MailDrop.erase(MailDrop.begin());
            //---------------------------------------
            // import this message into bigtable
            // refers to SampleRpcServer.cc -- generateEmails   putEmailsInBigTable
            
            for (int i=0; i< MailDrop.size(); i++) {

                EmailDTO email;
                string Mess;
                email.set_uid(MailDrop[i].MD5_id_mess);
                // int  Elements_collected = 0, lineNo;
                for (int lineNo =0; lineNo < MailDrop[i].content.size() //and Elements_collected != 3
                	; lineNo++) {
                    char TempLine[MAX_LEN_MESS]; strcpy(TempLine, MailDrop[i].content[lineNo].c_str());
                    char *token = strtok(TempLine, ":");
                    if (0 ==strcmp("Subject",token)){
                        token = strtok(NULL,":");
                        email.set_subject(token);
                        // Elements_collected++;
                    }else if (0 ==strcmp("From", token)){
                        token = strtok(NULL,":");
                        email.set_sender(token);
                        // Elements_collected++;
                    }else if (0 ==strcmp("Date", token)){
                        token = strtok(NULL,":");
                        email.set_date(token);
                        // Elements_collected++;
                    } else if (0 == strcmp("Message-ID", token)){}
                    else if (0 == strcmp("X-Forwarded-Message-Id", token)){}
                    else if (0 == strcmp("To", token)){}
                    else if (0 == strcmp("User-Agent", token)){}
                    else if (0 == strcmp("MIME-Version", token)){}
                    else if (0 == strcmp("In-Reply-To", token)){}
                    else if (0 == strcmp("Content-Type", token)){}
                    else if (0 == strcmp("Content-Language", token)){}
                    else if (0 == strcmp("Content-Transfer-Encoding", token)){}
                    else if (0 == strcmp("ContenTrsfer-Encoding", token)){}
                    else {Mess = Mess + MailDrop[i].content[lineNo];}
                }
                // if (Elements_collected!= 3){cerr<< "Missing Subject, From or Date from your Mbox format\n";exit(1);}
                // for (; lineNo< MailDrop[i].content.size(); lineNo++) Mess = Mess + MailDrop[i].content[lineNo];

                email.set_message(Mess);
                string serializedEmail = email.SerializeAsString();

                table_pt->put(username.c_str(),email.uid().c_str(), "email", serializedEmail.size(), serializedEmail.c_str());
                // cout << table_pt->get(username.c_str(), email.uid()) << "##########\n";
            }
            //-------------------------------------------
            flock(fileno(fd_pt_to_lock), LOCK_UN);
        }
        
        fclose(fd_pt_to_lock);
    }
    return 0;
}
