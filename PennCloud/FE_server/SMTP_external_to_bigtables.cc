//-----------------------------------------------------------------------
// SMTP_EXTERNAL_TO_BIGTABLE
#include "SMTP_external_to_bigtables.h"
// global variables
vector<int> Client_fds;
//vector<FILE *> Read_Write_fds;
vector<pthread_t> Threads; 
// vector<BigTable *> BIGTABLE_pts; // something wrong with string
char PARENTdir[MAX_LEN_PARENTdir];
bool DEBUG_flag = true;
MasterServiceClient masterServer(grpc::CreateChannel("localhost:50080", grpc::InsecureChannelCredentials()));
int main()
{  
    unsigned short port_id = 2500;//HANDOUT: but the default port number should now be 2500 (The default is 25, but port numbers below 1024 require root privileges to access.)

    // char ch; // control symbol
    // while (( ch = getopt( argc, argv, "p:v")) != -1) // refers to https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
    //     switch (ch) {
    //         case 'p': 
    //             port_id = atoi(optarg);
    //             break;
    //         case 'v': 
    //             DEBUG_flag = true;
    //             break;
    //     }
    // if (0 == argc - optind ){cerr << "Please provide DIRECTORY for mailboxes\n";exit(1);}
    // // void import_bigtables(const char *);
    // vector<string> BIGTABLE_names;
    // for (int i = optind; i <argc; i++) {
    // 	string DIR(argv[optind]);
    // 	// import_bigtables(DIR);
    //     if( find(BIGTABLE_names.begin(), BIGTABLE_names.end(), DIR) == BIGTABLE_names.end() ) 
    //         {BIGTABLE_pts.push_back(new BigTable(DIR)); BIGTABLE_names.push_back(DIR); }
    // }
    // // lecture 6 page 25
    
    // open a TCP port // get a tcp/ip socket
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd <0) {cerr << "Error connecting to socket):\t" << strerror(errno); exit(1);}
    
    void SignalHandler(int arg);
    signal(SIGINT,SignalHandler);
    const int enable = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {cerr << "setsockopt(SO_REUSEADDR) failed:\t" << strerror(errno); exit(1);}
    // SETSOCKOPT suggested by PIAZZA
    // https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));// this is an actual function
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port_id);
    bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)); // page 25 uses CONNECT; page 41 uses BIND
    // slide 6 page 29, 41
    listen(listen_fd, MAX_connection); // puts a socket into the listening state
    Client_fds.push_back(listen_fd);
    void *worker(void *arg); // DECLARATION
    while (true) { // what is the use of this loop?
        struct sockaddr_in clientaddr;
        socklen_t clientaddrlen = sizeof(clientaddr);
        int comm_fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
        pthread_t thread;       
        pthread_create(&thread, NULL, &worker, &comm_fd);
        Client_fds.push_back(comm_fd);
        if (DEBUG_flag)
        {// REQUIRED
            cout << '[' << comm_fd << "] New connection"  << "\n";}
        Threads.push_back(thread); // after pthread_create -- otherwise first thread has a problem
    }
    return 0;
}

//reply words seem can be arbitrary //https://tools.ietf.org/html/rfc821 (APPENDIX F) // according to /test/smtp-test.cc
char Welcome220[] = "220 localhost Service Ready\r\n";
char ServiceClosing[]="221 localhost Service closing transmission channel\r\n";
char OKsuccess250[]= "250 OK\r\n";
char StartInput[] = "354 Start mail input; end with <CRLF>.<CRLF>\r\n";
char RequrestdActionAbortedMboxOpening[] = "451 Requested action aborted: error in processing: MBOX opening error\r\n";    	
char RequrestdActionAbortedMboxLocked[] = "451 Requested action aborted: error in processing: MBOX already locked\r\n";     

char Unrecogn500[] = "500 Syntax error, command unrecognized\r\n"; 
char SyntaxErrPara501[]="501 Syntax error in parameters or arguments\r\n";
//reply words seem can be arbitrary //https://tools.ietf.org/html/rfc821
char BadSeq503[] ="503 Bad sequence of commands\r\n"; 
char NoSuchUser550[] = "550 No such user here\r\n";

void *worker(void *arg)
{
    void SignalHandler_thread(int arg); // DECLARATION
    signal(SIGUSR1,SignalHandler_thread); // SIGALRM
    //PIAZZA 147: since an interrupt will only be sent to an arbitrary thread ...
    sigset_t set; int sig; sigemptyset(&set);sigaddset(&set,SIGINT);
    if(pthread_sigmask(SIG_SETMASK, &set, NULL))
    {cerr << "empty TH: error pthread sigmask (SIGINT):" << strerror(errno); exit(2);} // block SIGINT signal

    const int comm_fd = *(int*) arg;
    EmailDTO email_to_insert;
    char MessContent[MAX_LEN_MESS];
    // send a simple message ----------------------
    
    char SenderName_AT_domain[MAX_USER_NAME];
    vector <string> ReceiverNameMboxes; //linhphan.mbox
    int STATE_this_mail_client = STATE_INITI;// keep commands in correct orders

    
    write(comm_fd, Welcome220, strlen(Welcome220));  
    if (DEBUG_flag) cout << Welcome220 << '\n';
    
    char commd[LEN_MAX_CMD];
    char buff[LEN_MAX_CMD],  mess2[2*LEN_MAX_CMD]; //message[2*LEN_MAX_CMD],
    memset(commd, 0, sizeof(char) * LEN_MAX_CMD);
    char HEAD[10], * LastPt; // LastPt is the pointer to an end of buff, in order to give COMMD correct command
    memset(buff, 0, sizeof(char) * LEN_MAX_CMD);
    memset(HEAD,0,sizeof(char) *20);
    
    
    bool TransmittingData = false;
    while(0 !=strcasecmp(HEAD, "QUIT") ){// handout: the command should be case -insensitive
        memset(commd, 0, sizeof(commd) ); //cout << "(AT the beginning) commd:" <<commd << "\tbuff:" << buff;
        while(NULL == (LastPt = strstr(buff,"\r\n") ) ) 
        {   strcat(commd, buff); 
            memset(buff, 0, sizeof(buff) );
            read(comm_fd, &buff, LEN_MAX_CMD - strlen(commd));  
        } 
        LastPt +=2;
        memset(mess2, 0, sizeof(mess2) );
	    strncpy(mess2, buff, LastPt - buff);
        strcat(commd, mess2 );
        strcpy(buff,LastPt);
        if (DEBUG_flag) cout << '[' <<comm_fd <<"] C: " << commd; // REQUIRED
        // finish dealing with COMMD and BUFF --  possibly BUFF still has the beginning of next message
        if (strlen(commd) >=4) 
        	memcpy(HEAD,commd,sizeof(char)*4);
	    
        if (TransmittingData)
        {
        	void Transmission_DATA(int &STATE_this_mail_client, const char *commd,const int comm_fd, bool &, EmailDTO &email, char *mess_content,vector <string> &);
        	Transmission_DATA(STATE_this_mail_client,commd,comm_fd, TransmittingData, email_to_insert, MessContent,ReceiverNameMboxes);
        }
        else if (0 == strcasecmp(HEAD, "HELO") )
        { 
        	if (STATE_INITI ==STATE_this_mail_client){
                bool have_parameter_separated_by_backspace = false;
                if (6 <strlen(commd)){
                    if (' ' != commd[4])// '' is different from '\b'
                    { 
                        write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
                        if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; 
                    }else{int idx=4;
                        for(; idx <strlen(commd) -2; idx++) if(' ' != commd[idx]) break;
                        if ( (strlen(commd)-2) !=idx) {have_parameter_separated_by_backspace = true; }
                        else{// 501 failure-- should have words following
                            write(comm_fd, SyntaxErrPara501 ,strlen(SyntaxErrPara501) );//strcat(message,mess2); // according to smtp-test -- I do not need mess2
                            if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << SyntaxErrPara501; 
                        }
                    }
                }else{// 501 failure -- should have words following
                    write(comm_fd, SyntaxErrPara501 ,strlen(SyntaxErrPara501) );//strcat(message,mess2); // according to smtp-test -- I do not need mess2
                    if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << SyntaxErrPara501; 
                }
                if (have_parameter_separated_by_backspace)
                {void handle_helo(int &STATE_this_mail_client, const char * commd,const int comm_fd,char *SenderName_AT_domain,
                vector <string> &ReceiverNameMboxes); // DECLARATION
            	   handle_helo(STATE_this_mail_client,commd, comm_fd,SenderName_AT_domain,ReceiverNameMboxes);
                }
            }else{// BAD sequence comands
                write(comm_fd,BadSeq503, strlen(BadSeq503) );  // not pointed out by RFC
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << BadSeq503; // REQUIRED
            }
        }
        else if (0 ==strcasecmp(HEAD, "MAIL") ) 
        {
        	void ParseMailFrom(int &STATE_this_mail_client,const char*commd,char *SenderName_AT_domain,const int comm_fd );
        	ParseMailFrom(STATE_this_mail_client,commd, SenderName_AT_domain,comm_fd );
        }
        else if (0 ==strcasecmp(HEAD, "RCPT") ) 
        {
        	void ParseRcptTo(int &STATE_this_mail_client, const char *commd,vector <string> &ReceiverNameMboxes,const int comm_fd);
        	ParseRcptTo(STATE_this_mail_client,commd,ReceiverNameMboxes,comm_fd);
        }
        else if (0 == strcasecmp(HEAD, "DATA") ) 
        {
            bool rest_are_backspaces_or_null = false;
            if (6 ==strlen(commd)) // commd is only "DATA\r\n"
                rest_are_backspaces_or_null = true;
            else if (' ' != commd[4])
            {write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; 
            }else{int idx=4;
                for(; idx <strlen(commd) -2; idx++) 
                    if(' ' != commd[idx]) break;
                if ( (strlen(commd)-2) ==idx) {rest_are_backspaces_or_null = true; }
                else{// 501 failure
                    write(comm_fd, SyntaxErrPara501 ,strlen(SyntaxErrPara501) );//strcat(message,mess2); // according to smtp-test -- I do not need mess2
                    if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << SyntaxErrPara501; 
                }
            }
            if (rest_are_backspaces_or_null)
        	{void WhetherStartData(int &STATE_this_mail_client,const char *SenderName_AT_domain,vector <string>&ReceiverNameMboxes, const int comm_fd, bool &, EmailDTO &email, char *mess_content);
                WhetherStartData(STATE_this_mail_client, SenderName_AT_domain,ReceiverNameMboxes, comm_fd, TransmittingData, email_to_insert, MessContent);
            }
		}
        else if (0 == strcasecmp(HEAD, "QUIT") )
        {bool correctHEAD = false;
            if (6 ==strlen(commd)) // commd is only "QUIT\r\n"
                {correctHEAD = true;}
            else if (' ' != commd[4])
            {
                write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; 
            }// no 501 failure
            if (correctHEAD)
            {memset(SenderName_AT_domain,0,sizeof(SenderName_AT_domain));
                ReceiverNameMboxes.clear();
                write(comm_fd,ServiceClosing, strlen(ServiceClosing) );
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << ServiceClosing; // REQUIRED
            }
		}
        else if (0 == strcasecmp(HEAD, "RSET") ) 
        {
            bool rest_are_backspaces_or_null = false;
        	if (6 ==strlen(commd)) // commd is only "RSET\r\n"
                rest_are_backspaces_or_null = true;
            else if (' ' != commd[4])
            {
                write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; 
            }else{int idx=4;
                for(; idx <strlen(commd) -2; idx++) 
                    if(' ' != commd[idx]) break;
                if ( (strlen(commd)-2) ==idx) {rest_are_backspaces_or_null = true; }
                else{// 501 failure -- should have no parameter/ argument
                    write(comm_fd, SyntaxErrPara501 ,strlen(SyntaxErrPara501) );//strcat(message,mess2); // according to smtp-test -- I do not need mess2
                    if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << SyntaxErrPara501; 
                }
            }
            if (rest_are_backspaces_or_null)
        	{
                STATE_this_mail_client = 0;
                void handle_helo(int &STATE_this_mail_client, const char * commd,const int comm_fd,char *SenderName_AT_domain,
                    vector <string> &ReceiverNameMboxes); // DECLARATION
            	handle_helo(STATE_this_mail_client, commd, comm_fd,SenderName_AT_domain, ReceiverNameMboxes); // HEAD RSET, HELO will not be distinguished
            }
        }
        else if (0 == strcasecmp(HEAD, "NOOP") ) 
        {
        	bool rest_are_backspaces_or_null = false;
            if (6 ==strlen(commd)) 
                rest_are_backspaces_or_null = true;// commd is only "NOOP\r\n"
            else if (' ' != commd[4])
            {
                write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; 
            }// no need to check 501 parameter failure
            if (rest_are_backspaces_or_null){
                write(comm_fd, OKsuccess250, strlen(OKsuccess250)); // failure 500 is hard to realize here
        	   if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << OKsuccess250; // REQUIRED
            }
        }
        else {write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
	        if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; // REQUIRED
	    }
    }
    int pos_thread = 0;
    for (;pos_thread< Threads.size(); pos_thread ++)// use Client_fds to locate it
        if (comm_fd == Client_fds[pos_thread+1])
            break;
    if ( Threads.size() == pos_thread)
    {cerr << "Error locating pos_thread within Client_fds\n";exit(1);}
    else
    {
        Threads.erase(Threads.begin() + pos_thread );
        Client_fds.erase(Client_fds.begin() + pos_thread + 1) ;
    }

    close(comm_fd);
    if (DEBUG_flag) cout<< '['<< comm_fd << "] Connection closed\n"; //REQUIRED
    pthread_exit(NULL); // the server shold also properly clean up its resources by terminating pthreads when their connection closes
    pthread_detach(pthread_self() );
}



void SignalHandler(int arg)
{// once SIGINT is received
    char Err_ShutDown421[] = "421 Service unavailable\r\n";
    if (0== Threads.size())
    {
        if (DEBUG_flag) cout << "No client at all\n";
        exit(1);
    }
    else
    {
        vector <bool> pthread_kill_success_flag;
        for(int i =0;i<Threads.size();i++)           
        {
            write(Client_fds[i+1], Err_ShutDown421, strlen(Err_ShutDown421));
            close(Client_fds[i+1]);// close the file descriptor -- if you forget this PTHREAD_KILL may cause segmentation default
            // // blog.csdn.net/yusiguyuan/article/details/14230719
            // // This is suggested by Yuding Ai
            int rc = pthread_kill(Threads[i], SIGUSR1); // SIGALRM
            if (0 == rc) //pthread_ kill successfully send the signal
                pthread_kill_success_flag.push_back(true);
            else
                pthread_kill_success_flag.push_back(false);

        }
        

        for(int i =0;i<Threads.size();i++)
            if (pthread_kill_success_flag[i]) {int rc = pthread_join(Threads[i], NULL);}
        close(Client_fds[0]); // close listen_fd
    
        // for(int i=0; i< BIGTABLE_pts.size(); i++)
        //     delete BIGTABLE_pts[i];
        exit(1);
    }
}
void SignalHandler_thread(int arg){
    // for (int i =0; i< FD_pts_to_UNLOCK.size(); i++) // does not work
    //     if (NULL != FD_pts_to_UNLOCK[i])
    //     {flock(FD_pts_to_UNLOCK[i],LOCK_UN);fclose(FD_pts_to_UNLOCK[i]); FD_pts_to_UNLOCK[i] =NULL;}
    pthread_exit(NULL);
}

void handle_helo(int &STATE_this_mail_client, const char * commd,const int comm_fd,char *SenderName_AT_domain,
    vector <string> &ReceiverNameMboxes)
{        	
	if (STATE_INITI ==STATE_this_mail_client){
        bool have_parameter_separated_by_backspace = false;
        if (6 <strlen(commd)){
            if (' ' != commd[4])// '' is different from '\b'
            {
                write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; 
            }else{int idx=4;
                for(; idx <strlen(commd) -2; idx++) if(' ' != commd[idx]) break;
                if ( (strlen(commd)-2) !=idx) {have_parameter_separated_by_backspace = true; }
                else{// 501 failure-- should have words following
                    write(comm_fd, SyntaxErrPara501 ,strlen(SyntaxErrPara501) );//strcat(message,mess2); // according to smtp-test -- I do not need mess2
                    if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << SyntaxErrPara501; 
                }
            }
        }else{// 501 failure -- should have words following
            write(comm_fd, SyntaxErrPara501 ,strlen(SyntaxErrPara501) );//strcat(message,mess2); // according to smtp-test -- I do not need mess2
            if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << SyntaxErrPara501; 
        }
        if (have_parameter_separated_by_backspace)
        {
            STATE_this_mail_client =1;
            memset(SenderName_AT_domain,0,sizeof(SenderName_AT_domain));
            ReceiverNameMboxes.clear();
            char temp[]="250 localhost\r\n"; // at least \r\n
    	// according to smtp-test -- I do not need mess2 PIAZZA @149
    	 	write(comm_fd, temp ,strlen(temp) );//strcat(message,mess2); 
    		if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << temp; // REQUIREDs
        }
	}
	else{write(comm_fd,BadSeq503, strlen(BadSeq503) );  // not pointed out by RFC
		if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << BadSeq503; // REQUIRED
	}
}
//----------------------------------------------
void ParseMailFrom(int &STATE_this_mail_client,const char *commd, char *SenderName_AT_domain ,const int comm_fd)
{
	char HEAD[20]; memset(HEAD,0,sizeof(HEAD));
	memcpy(HEAD,commd,sizeof(char)*10);

	if (0 == strcasecmp(HEAD, "MAIL FROM:") )
	{
		if (STATE_HELO_MAIL == STATE_this_mail_client)
		{
			STATE_this_mail_client = 2;
			char mess2[LEN_MAX_CMD];
            memset(mess2,0,sizeof(mess2));
            strncpy(mess2,commd+10,strlen(commd)-10);
            char * pch = strchr(mess2, '<'); // I did not debug this one
			pch ++;
			int lengName = strchr(mess2, '>') - pch; 
            if (0 < lengName){ // PIAZZA @184 I should  case 0 is acceptable == length  -- recall default of SenderName_AT_domain = '\0'
                memset(SenderName_AT_domain,0,sizeof(SenderName_AT_domain));
                strncpy(SenderName_AT_domain,pch, lengName*sizeof(char));
              }
            write(comm_fd,OKsuccess250,strlen(OKsuccess250));
            if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << OKsuccess250 ;
		}
		else {write(comm_fd, BadSeq503,strlen(BadSeq503));
			if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << BadSeq503; // REQUIRED
		}
	}
	else {//strcpy(message, Unrecogn500);strcat(message, mess2); 
		write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
        if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; // REQUIRED
	}
}
				


void ParseRcptTo(int &STATE_this_mail_client, const char *commd, vector <string> &ReceiverNameMboxes,const int comm_fd)
{
	char OKsuccess[]= "250 OK\r\n";
	char HEAD[20];
	memset(HEAD,0,sizeof(HEAD));
	memcpy(HEAD,commd,sizeof(char)*8);
	if (0 == strcasecmp(HEAD, "RCPT TO:"))
	{
		if (STATE_MAIL_RCPT == STATE_this_mail_client || STATE_RCPT_DATA == STATE_this_mail_client) // Possible to have many RCPT TO: 
		{STATE_this_mail_client = STATE_RCPT_DATA;
            char mess2[LEN_MAX_CMD];
            memset(mess2,0,sizeof(mess2));
            strncpy(mess2,commd+8,strlen(commd)-8);
			char * pch = strchr(mess2, '<'); // EACH LINE only one receipt
			pch ++;
			int lengName = strchr(mess2, '>') - pch; // PIAZZA @184 I should consider case 0 == length
			if (0 < lengName) // INTIAL 
			{
				char RCPTName[MAX_USER_NAME];
				strncpy(RCPTName,pch, lengName*sizeof(char));
				int lenRCName = strchr(RCPTName, '@')  - RCPTName; // I have not DEBUG this one
				char RCPTMbox1[MAX_USER_NAME];
				memset(RCPTMbox1,0,sizeof(RCPTMbox1));
				strncpy(RCPTMbox1,RCPTName,lenRCName*sizeof(char));
                ReceiverNameMboxes.push_back(string(RCPTMbox1));
                write(comm_fd,OKsuccess250,strlen(OKsuccess250));
                if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << OKsuccess250;
// ----------------------------------
			}
			else{ //PIAZZA @184 I should consider case 0 == length 
				if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: EmptySender \n"; // REQUIRED
                exit(1);
			}
		}else {write(comm_fd, BadSeq503,strlen(BadSeq503));
			if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << BadSeq503; // REQUIRED
		}
	}
	else {// 500 failure
		write(comm_fd,Unrecogn500, strlen(Unrecogn500) );
        if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << Unrecogn500; // REQUIRED
    }
}

void WhetherStartData(int &STATE_this_mail_client,const char *SenderName_AT_domain, vector <string>&ReceiverNameMboxes,
    const int comm_fd, bool &TransmittingData, EmailDTO &email, char *mess_content)
{
	if (STATE_RCPT_DATA == STATE_this_mail_client)
	{// 5 is included due to example on page 37 of RFC 821 ?? However, according to PIAZZA
        STATE_this_mail_client = STATE_DATA;
		TransmittingData = true;

        // write first line -- it is required and it related with uniqueness of MD5 ID POP3

        char RequiredLine[LEN_MAX_CMD];
        memset(RequiredLine,0,sizeof(RequiredLine));
        strcpy(RequiredLine,"From <");
        strcat(RequiredLine, SenderName_AT_domain);
        strcat(RequiredLine,"> ");
        time_t CurrT = time(0);
        string timestamp = ctime(&CurrT); // end with \n\0
        
        strcat(RequiredLine,timestamp.c_str());
        RequiredLine[strlen(RequiredLine)-1] = '\r';
        RequiredLine[strlen(RequiredLine)] = '\n';
        
        char MD5_id_mess[2*MD5_DIGEST_LENGTH+1];
        memset(MD5_id_mess,0,sizeof(MD5_id_mess));
                        
        unsigned char digest[2*MD5_DIGEST_LENGTH+1];
        computeDigest(RequiredLine,strlen(RequiredLine),digest);
        //-------------------------------------
        // PIAZZA @252: You should convert MD5 hash to a string for UIDL in whatever way you like. 
        // A hex string might be the easiest one and satisfies the range requirements.

        //https://stackoverflow.com/questions/5661101/how-to-convert-an-unsigned-character-array-into-a-hexadecimal-string-in-c
        char BUFF[32];
        for (int j=0; j < MD5_DIGEST_LENGTH; j++){
            sprintf(BUFF, "%02x", digest[j]);// *2 == cannot connect to Thunderbird
            strcat(MD5_id_mess,BUFF);
        }
        email.set_uid(MD5_id_mess);

        memset(mess_content,0,sizeof(mess_content));
        write(comm_fd,StartInput, strlen(StartInput));
        if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << StartInput; // REQUIRED
        
	}else{write(comm_fd, BadSeq503, strlen(BadSeq503));
		if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << BadSeq503; // REQUIREDs
	}
}

void Transmission_DATA(int &STATE_this_mail_client, const char *commd,const int comm_fd, bool &TransmittingData,EmailDTO &email,char *mess_content,vector <string> &ReceiverNameMboxes)
{
	if (4 == STATE_this_mail_client)
	{
        char inputline[LEN_MAX_CMD]; // Give a new name for variable COMMD -- a protection
        memset(inputline,0,sizeof(inputline));
        strcpy(inputline,commd);
        if ( 0 == strcasecmp(inputline, ".\r\n")) {
            TransmittingData = false; // finishing transmitting data
            STATE_this_mail_client = 1; // According to PIAZZA @208, return to HELO state, but I think this is not consistent with SCENARIO 10 on page 38 of RFC
            email.set_message(mess_content);
            //------------------
            // for (int bt_pt_id=0; bt_pt_id <BIGTABLE_pts.size(); bt_pt_id ++)
            for (int rec_id =0; rec_id < ReceiverNameMboxes.size(); rec_id ++)
            {
                email.set_receiver(ReceiverNameMboxes[rec_id].c_str());
                // string uid = HttpService::gen_rand_string(15);
                // email.set_uid(uid);
                // put it global variable -- MasterServer
                WriteResponse response = masterServer.RequestWrite(email.receiver(), email.uid(), email.SerializeAsString(), WriteType::PUT, FileType::EMAIL);
                // string serializedEmail = email.SerializeAsString();
                // if (DEBUG_flag)printf("[%d] #########################################\n%s#########################################3333\n",,commd_fd,email.message().c_str());
                // BIGTABLE_pts[bt_pt_id]->put(email.receiver(), email.uid(),"email", serializedEmail.size(),serializedEmail.c_str());
            }
            //------------------
            write(comm_fd,OKsuccess250,strlen(OKsuccess250));
            if (DEBUG_flag) cout << '[' <<comm_fd <<"] S: " << OKsuccess250; // REQUIRED
        }else{
            // printf("#################################333333");
            char TempLine[MAX_LEN_MESS]; strcpy(TempLine, commd);
            char *token = strtok(TempLine, ":");
            if (0 ==strcmp("Subject",token)){
                token = strtok(NULL,":");
                email.set_subject(token);
                // Elements_collected++;
            }else if (0 ==strcmp("From", token)){
                token = strtok(NULL,":");
                printf("sender::%s---------------------\n",token);
                email.set_sender(token);
                // Elements_collected++;
            }else if (0 ==strcmp("Date", token)){
                token = strtok(NULL,":");
                printf("Date::%s---------------------\n",token+4);
                email.set_date(token);
                // Elements_collected++;
            }else if (0 ==strncmp("Date", token,4)){
                // token = strtok(NULL,":");
                printf("Date::%s---------------------\n",token+4);
                email.set_date(token+4);
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
            else {strcat(mess_content,inputline);}
        } 
    } else{// I think this part is never implemented

    }
}