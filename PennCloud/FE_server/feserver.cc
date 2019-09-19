/*
 * feserver.cc
 * PennCloud CIS 505
 * Author T01
 * Date: Nov 8 2017
 */

#include "feserver.h"
//-------------------------------------------
FEServer::FEServer(const string &host, int port){
    this->port = port;
    this->host = host;

}

FEServer::~FEServer(){
}

typedef struct{
    MasterServiceClient* masterServer;
    bool db;
    int connfd;
} WorkerArg;

typedef struct {
	bool db;
	int connfd;
	string redirectAddress;
} LoadBalancerArg;

typedef struct {
	bool db;
	int connfd;
	AdminConsoleRepositoryClient* adminClient;
	MasterServiceClient* masterServer;
} AdminConsoleArg;

//int FEServer::run(){
int FEServer::run(MasterServiceClient &masterServer){
    int listenfd, connfd;
    struct sockaddr_in servaddr; //server address info
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //convert from host to network
    servaddr.sin_port = htons(port);

    // next following the order: socket(), bind(), listen() and accept()
    // create a TCP stream socket
    if((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        exit(2);
    }

    // port reuse
    int enable = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        fprintf(stderr, "setsocket(): %s\n", strerror(errno));
        exit(3);
    }

    // bind the listenfd
    if((bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)))==-1){
        fprintf(stderr, "bind(): %s\n", strerror(errno));
        close(listenfd);
        exit(4);
    }

    // listen to incoming traffic
    if((listen(listenfd, Max_conn_capacity))== -1){
        fprintf(stderr, "listen(): %s\n", strerror(errno));
        close(listenfd);
        exit(5);
    }

    mainThread = pthread_self();
    while(1){
        int err;
        struct sockaddr_in cliaddr; //client addr info
        socklen_t clilen = sizeof(cliaddr);

        //Master thread accept the connectoin
        if((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen))<0){
            if(errno == EINTR){
                // EINTR is caused by SIGINT, so close listenfd and get out of the loop
                close(listenfd);
                break;
            } else {
                fprintf(stderr, "accept(): %s\n", strerror(errno));
            }
        }        

        // print welcome massage
//        cout<< "Serving HTTP on port " << port << endl;
        //-----------------
        //creat worker thread and hand the 'hard and dirty' work to them
        //-----------------
        pthread_t thread;
        WorkerArg arg;
        arg.connfd = connfd;
        arg.db = db;
        arg.masterServer = &masterServer;

        err = pthread_create(&thread, NULL, &worker, &arg);
        if(err != 0){
            fprintf(stderr,"can't create thread");
            exit(1);
        }

        threads.insert(thread); 
    }

    for(pthread_t i : threads){ 
        // wait till all alive workers to finish
        pthread_join(i,NULL);
    }

    close(listenfd);
    return 0;
}

string getRedirectAddress(long &counter, LoadBalancerData &loadBalancerData) {
	int index = counter++ % loadBalancerData.feServers.size();

	lock_guard<mutex> lock(loadBalancerData.mtx);
	ServerStatusDTO server = loadBalancerData.feServers[index];
	while (server.status() == ServerStatus::UNKNOWN) {
		index = counter++ % loadBalancerData.feServers.size();

		server = loadBalancerData.feServers[index];
	}

	return server.ipaddress() + ":" + to_string(server.port());
}

int FEServer::runLoadBalancer(LoadBalancerData &loadBalancerData) {
	int listenfd, connfd;
	struct sockaddr_in servaddr; //server address info
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //convert from host to network
	servaddr.sin_port = htons(port);

	// next following the order: socket(), bind(), listen() and accept()
	// create a TCP stream socket
	if((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "socket(): %s\n", strerror(errno));
		exit(2);
	}

	// port reuse
	int enable = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		fprintf(stderr, "setsocket(): %s\n", strerror(errno));
		exit(3);
	}

	// bind the listenfd
	if((bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)))==-1){
		fprintf(stderr, "bind(): %s\n", strerror(errno));
		close(listenfd);
		exit(4);
	}

	// listen to incoming traffic
	if((listen(listenfd, Max_conn_capacity))== -1){
		fprintf(stderr, "listen(): %s\n", strerror(errno));
		close(listenfd);
		exit(5);
	}

	long counter = 0;
	while(1){
		int err;
		struct sockaddr_in cliaddr; //client addr info
		socklen_t clilen = sizeof(cliaddr);

		//Master thread accept the connection
		if((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen))<0){
			if(errno == EINTR){
				// EINTR is caused by SIGINT, so close listenfd and get out of the loop
				close(listenfd);
				break;
			} else {
				fprintf(stderr, "accept(): %s\n", strerror(errno));
			}
		}

		// print welcome massage
		cout<< "Running load balancer on port " << port << endl;
		//-----------------
		//create worker thread and hand the 'hard and dirty' work to them
		//-----------------
		pthread_t thread;
		LoadBalancerArg arg;
		arg.connfd = connfd;
		arg.db = db;
		arg.redirectAddress = getRedirectAddress(counter, loadBalancerData);

		err = pthread_create(&thread, NULL, &loadBalancer, &arg);
		if(err != 0){
			fprintf(stderr,"can't create thread");
			exit(1);
		}

		threads.insert(thread);
	}

	for(pthread_t i : threads){
		// wait till all alive workers to finish
		pthread_join(i,NULL);
	}

	close(listenfd);

	return 0;
}

int FEServer::runAdminConsole(AdminConsoleData &adminConsoleData) {
	int listenfd, connfd;
	struct sockaddr_in servaddr; //server address info
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //convert from host to network
	servaddr.sin_port = htons(port);

	// next following the order: socket(), bind(), listen() and accept()
	// create a TCP stream socket
	if((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "socket(): %s\n", strerror(errno));
		exit(2);
	}

	// port reuse
	int enable = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		fprintf(stderr, "setsocket(): %s\n", strerror(errno));
		exit(3);
	}

	// bind the listenfd
	if((bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)))==-1){
		fprintf(stderr, "bind(): %s\n", strerror(errno));
		close(listenfd);
		exit(4);
	}

	// listen to incoming traffic
	if((listen(listenfd, Max_conn_capacity))== -1){
		fprintf(stderr, "listen(): %s\n", strerror(errno));
		close(listenfd);
		exit(5);
	}

	long counter = 0;
	while(1){
		int err;
		struct sockaddr_in cliaddr; //client addr info
		socklen_t clilen = sizeof(cliaddr);

		//Master thread accept the connection
		if((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen))<0){
			if(errno == EINTR){
				// EINTR is caused by SIGINT, so close listenfd and get out of the loop
				close(listenfd);
				break;
			} else {
				fprintf(stderr, "accept(): %s\n", strerror(errno));
			}
		}

		// print welcome massage
		cout<< "Running admin console on port " << port << endl;
		//-----------------
		//create worker thread and hand the 'hard and dirty' work to them
		//-----------------
		pthread_t thread;
		AdminConsoleArg arg;
		arg.connfd = connfd;
		arg.db = db;
		arg.adminClient = adminConsoleData.adminConsoleRepository;
		arg.masterServer = adminConsoleData.masterServer;

		err = pthread_create(&thread, NULL, &adminConsoleWorker, &arg);
		if(err != 0){
			fprintf(stderr,"can't create thread");
			exit(1);
		}

		threads.insert(thread);
	}

	for(pthread_t i : threads){
		// wait till all alive workers to finish
		pthread_join(i,NULL);
	}

	close(listenfd);

	return 0;
}

//==================
//For Sig handling
//==================
static void setmask(){
    int err;
    sigset_t mask;
    sigaddset(&mask, SIGINT);
    if((err = pthread_sigmask(SIG_SETMASK, &mask, NULL)) != 0){
        fprintf(stderr,"sigblock error");
        exit(1);
    }
}

static void interrupt(int connfd){
    const char *Close;
    Close = "-ERR Server shutting down\r\n";
    write(connfd,Close, strlen(Close));
    if(db){
        // Customize debug message
        fprintf(stderr,"[%d] S: Worker thread tid = [%ld] revieved SIGALRM and close connection\t\n",connfd,pthread_self());
    }
}

//===========================
// Worker thread
//===========================
static void *worker(void *arg){
    //---------------------
    // deal with signal
    //---------------------
    setmask();
    // Now httpworker thread won't revieve SIGINT anymore

    WorkerArg p= *(WorkerArg *) arg;

    // Creat a service obj;
    int connfd = p.connfd;
    bool debug = p.db;
    // SMTP_external_to_bigtables(p.connfd, p.db, p.masterServer);
    HttpService Hservice = HttpService(p.connfd, p.db, p.masterServer);

    //----------------------
    // Read from Client
    //----------------------
    long bytes_read = 0;
    long total_read = 0;
    vector<char> buf(Max_cmd);
    long offset = 0;

    bool isPost = false;
    long contentLength = -1;
    string test = "";
    cerr << "Entering read loop......" << endl;
    while(1) {
    	//keep recv/read
    	cerr<< "inside loop bufsize is "<<buf.size()<<endl;
    	if ((bytes_read = read(connfd, buf.data() + offset, buf.size() - offset)) < 0) {
    		if (errno == EINTR) {
    			// EINTR is tirggered by SIGALRM, so handle quit !
    			interrupt(connfd);
    			// get out of the loop
    			break;
    		} else {
    			/// error:
    			fprintf(stderr, "read(): %s\n", strerror(errno));
    			close(connfd); // close the connection
    			pthread_exit(0);
    		}

    	} else if (bytes_read == 0) {
    		// no data available
    		if (offset == 0) {
    			/// error:
    			fprintf(stderr, "read(): %s\n", strerror(errno));
    			close(connfd); // close the connection
    			pthread_exit(0);
    		} else {
    			/// break;
    			int count;
    			usleep(1000);
    			ioctl(connfd, FIONREAD, &count);
    			if(count > 0){
    				cerr<< "continuing byte = 0------" << endl;
    				continue;
    			}
    			test = "bytes_read is 0";
    			break;
    		}
    	} else {
    		if (bytes_read == buf.size() - offset){
    			total_read += bytes_read;
    			offset += bytes_read;

    			// If this is a POST, determine if we read all the content
    			isPost = strncmp(buf.data(), "POST", 4) == 0;
    			if (isPost && contentLength < 0) {
    				cerr << "Checking for Content-Length: " << endl;
    				char* start = strstr(buf.data(), "Content-Length: ");
    				char* end = strstr(start, "\r\n");
    				string conLenLine(start, end - start);
    				int pos = conLenLine.find(" ");

    				cerr << "ContentLength line: '" << conLenLine << "'" << endl;
    				cerr << "conLenLine.substr(pos + 1): " << conLenLine.substr(pos + 1) << endl;
    				contentLength = stoi(conLenLine.substr(pos + 1));
    				cerr << "Content length value : " << contentLength << endl;
    			}

    			if (isPost) {
    				char* contentStart = strstr(buf.data(), "\r\n\r\n");
    				int nonContentLength = contentStart - buf.data();
    				long contentBytesRead = total_read - nonContentLength;

    				cerr << "Content start: " << &contentStart << endl;
    				char* ptr = buf.data();
    				cerr << "buf start: " << &(ptr) << endl;
    				cerr << "nonContent length: " << nonContentLength << endl;
    				cerr << "Total bytes read: " << total_read << endl;

    				// Subtract 4 for the beginning and ending '\r\n\r\n'
    				contentBytesRead -= 4;

    				cerr << "Expected content length: " << contentLength << " Actual content length read: " << contentBytesRead << endl;
    				if (contentLength != contentBytesRead) {
    					string bufStr(buf.data(), total_read);
    					cerr << "--------------------------------------------------------------------------------------------------" << endl;
    					cerr << bufStr << endl;
    					cerr << "--------------------------------------------------------------------------------------------------" << endl;
    					cerr << "Trying to read more content..." << endl;
    					if (buf.capacity() < contentLength) {
    						buf.resize(buf.capacity() + contentLength);
    						cerr << "Increased buf size to " << buf.capacity() << endl;
    					}
    					continue;
    				}
    			}
    			else {
    				cerr<< " Increasing the size of buffer" << endl;
    				buf.resize(2 * buf.size());
    			}
    		} else {
    			total_read += bytes_read;
    			offset += bytes_read;

    			// If this is a POST, determine if we read all the content
    			isPost = strncmp(buf.data(), "POST", 4) == 0;
    			if (isPost && contentLength < 0) {
    				cerr << "Checking for Content-Length: " << endl;
    				char* start = strstr(buf.data(), "Content-Length: ");
    				char* end = strstr(start, "\r\n");
    				string conLenLine(start, end - start);
    				int pos = conLenLine.find(" ");

    				cerr << "ContentLength line: '" << conLenLine << "'" << endl;
    				cerr << "conLenLine.substr(pos + 1): " << conLenLine.substr(pos + 1) << endl;
    				contentLength = stoi(conLenLine.substr(pos + 1));
    				cerr << "Content length value : " << contentLength << endl;
    			}

    			if (isPost) {
    				char* contentStart = strstr(buf.data(), "\r\n\r\n");
    				int nonContentLength = contentStart - buf.data();
    				long contentBytesRead = total_read - nonContentLength;

    				cerr << "Content start: " << &contentStart << endl;
    				char* ptr = buf.data();
    				cerr << "buf start: " << &(ptr) << endl;
    				cerr << "nonContent length: " << nonContentLength << endl;
    				cerr << "Total bytes read: " << total_read << endl;

    				// Subtract 4 for the beginning and ending '\r\n\r\n'
    				contentBytesRead -= 4;

    				cerr << "Expected content length: " << contentLength << " Actual content length read: " << contentBytesRead << endl;
    				if (contentLength != contentBytesRead) {
    					string bufStr(buf.data(), total_read);
    					cerr << "--------------------------------------------------------------------------------------------------" << endl;
    					cerr << bufStr << endl;
    					cerr << "--------------------------------------------------------------------------------------------------" << endl;
    					cerr << "Trying to read more content..." << endl;
    					if (buf.capacity() < contentLength) {
    						buf.resize(buf.capacity() + contentLength);
    						cerr << "Increased buf size to " << buf.capacity() << endl;
    					}
    					continue;
    				}
    			}

    			break;
    		}
    	}
    	//        cerr<< "bytes_read" << bytes_read<<" offset is " << offset << endl;
    	total_read += bytes_read;
    	//        cerr<< "buf size is" << buf.size()<<" total_read is " << total_read << endl;
    }

    //-----------------------------------
    // Expecting CMD HttpService here for now
    //-----------------------------------
    //-----------------
    // print debug msg
    //-----------------
    if(debug) fprintf(stderr, "[%d] C: %s", connfd, buf.data());

    //---------------
    //handle command
    //---------------
    string s_buf(buf.data(), total_read);
    Hservice.HandleCMD(s_buf);

//    cerr<< buf.max_size()<<endl;
//    cerr<<  "buf_size is "<< buf.size() << " total_read = " << total_read<<endl;
//    cerr<<  "exit the loop because: "<< test << endl;
    // After handled the command close connection
    pthread_detach(pthread_self());
    threads.erase(pthread_self());
    if(debug) fprintf(stderr, "[%d] C: Connection closed\r\n", connfd);
    close(connfd); // close the connection
    pthread_exit(0);

}

//===========================
// Worker thread
//===========================
static void *loadBalancer(void *arg){
    //---------------------
    // deal with signal
    //---------------------
    setmask();
    // Now httpworker thread won't revieve SIGINT anymore

    LoadBalancerArg loadBalancerArg = *(LoadBalancerArg *) arg;

    // Creat a service obj;
    int connfd = loadBalancerArg.connfd;
    bool debug = loadBalancerArg.db;
    HttpService Hservice = HttpService(loadBalancerArg.connfd, loadBalancerArg.db, NULL);

    //----------------------
    // Read from Client
    //----------------------
    int bytes_read;
    int bytes_left = Max_cmd; //Max_cmd = 5120
    char buf[Max_cmd];
    char * ptr, * pptr;
    // initialize buf and ptr
    memset(&buf, 0, sizeof(buf));
    ptr = buf;

    while(1){
        //keep recv/read
        if((bytes_read = read(connfd, ptr, sizeof(buf))) < 0){
            if(errno == EINTR){
                // EINTR is tirggered by SIGALRM, so handle quit !
                interrupt(connfd);
                // get out of the loop
                break;
            }else{
                fprintf(stderr, "read(): %s\n", strerror(errno));
                close(connfd); // close the connection
                pthread_exit(0);
            }
        }else if(bytes_read == 0 ){
            pthread_detach(pthread_self());
            threads.erase(pthread_self());
            close(connfd); // close the connection
            pthread_exit(0);
        }

        bytes_left -= bytes_read;
        ptr += bytes_read; // proceed the char pointer to the end
        if(bytes_left <= 0){
            fprintf(stderr, "input too long(exceed 5120 chars!) \r\n");

            //reset buffer
            bytes_left = Max_cmd;
            memset(&buf, 0, sizeof(buf));
            ptr = buf;
            continue;
        }

        //-----------------------------------
        // Expecting CMD HttpService here for now
        //-----------------------------------
        // if finds a "\r\n\r\n" then handle the command
        while((pptr = strstr(buf,http_cmd_term.c_str())) != NULL){
            //-----------------
            // print debug msg
            //-----------------
            if(debug) fprintf(stderr, "[%d] C: %s", connfd, buf);

            //---------------
            //handle command
            //---------------
            int st;

            // Handle redirect with the provided redirect address
            st = Hservice.handleHttpRedirect(buf, "http://" + loadBalancerArg.redirectAddress);

            // After handled the command close connection
            pthread_detach(pthread_self());
            threads.erase(pthread_self());
            if(debug) fprintf(stderr, "[%d] C: Connection closed\r\n", connfd);
            close(connfd); // close the connection
            pthread_exit(0);
        }

    }

    if(debug ) fprintf(stderr, "[%d] C: Connection closed\r\n", connfd);
    close(connfd); // close the connection
    pthread_exit(0);
}

//===========================
// Worker thread
//===========================
static void *adminConsoleWorker(void *arg){
    //---------------------
    // deal with signal
    //---------------------
    setmask();
    // Now httpworker thread won't revieve SIGINT anymore

    AdminConsoleArg adminConsoleArg = *(AdminConsoleArg *) arg;

    // Creat a service obj;
    int connfd = adminConsoleArg.connfd;
    bool debug = adminConsoleArg.db;
    HttpService httpService = HttpService(adminConsoleArg.connfd, adminConsoleArg.db, adminConsoleArg.masterServer);

    httpService.setLogin(true);
    httpService.setAdminClient(adminConsoleArg.adminClient);

    //----------------------
    // Read from Client
    //----------------------
    int bytes_read;
    int bytes_left = Max_cmd; //Max_cmd = 5120
    char buf[Max_cmd];
    char * ptr, * pptr;
    // initialize buf and ptr
    memset(&buf, 0, sizeof(buf));
    ptr = buf;

    while(1){
        //keep recv/read
        if((bytes_read = read(connfd, ptr, sizeof(buf))) < 0){
            if(errno == EINTR){
                // EINTR is tirggered by SIGALRM, so handle quit !
                interrupt(connfd);
                // get out of the loop
                break;
            }else{
                fprintf(stderr, "read(): %s\n", strerror(errno));
                close(connfd); // close the connection
                pthread_exit(0);
            }
        }else if(bytes_read == 0 ){
            pthread_detach(pthread_self());
            threads.erase(pthread_self());
            close(connfd); // close the connection
            pthread_exit(0);
        }

        bytes_left -= bytes_read;
        ptr += bytes_read; // proceed the char pointer to the end
        if(bytes_left <= 0){
            fprintf(stderr, "input too long(exceed 5120 chars!) \r\n");

            //reset buffer
            bytes_left = Max_cmd;
            memset(&buf, 0, sizeof(buf));
            ptr = buf;
            continue;
        }

        //-----------------------------------
        // Expecting CMD HttpService here for now
        //-----------------------------------
        // if finds a "\r\n\r\n" then handle the command
        while((pptr = strstr(buf,http_cmd_term.c_str())) != NULL){
            //-----------------
            // print debug msg
            //-----------------
            if(debug) fprintf(stderr, "[%d] C: %s", connfd, buf);

            //---------------
            //handle command
            //---------------
            int st;

            // Handle the command
            st = httpService.HandleCMD(buf);

            // After handled the command close connection
            pthread_detach(pthread_self());
            threads.erase(pthread_self());
            if(debug) fprintf(stderr, "[%d] C: Connection closed\r\n", connfd);
            close(connfd); // close the connection
            pthread_exit(0);
        }

    }

    if(debug ) fprintf(stderr, "[%d] C: Connection closed\r\n", connfd);
    close(connfd); // close the connection
    pthread_exit(0);
}

bool isMessageComplete(vector<char>) {

}

size_t getSizeOfMessage(vector<char>){

}


