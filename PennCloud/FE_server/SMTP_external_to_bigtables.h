
// SMTP_EXTERNAL_TO_BIGTABLE
#ifndef SMTP_EXTERNAL_TO_BIGTABLE_2_H_
#define SMTP_EXTERNAL_TO_BIGTABLE_2_H_
#include <stdlib.h>
#include <stdio.h>

#include <iostream>

#include<string.h>
#include <errno.h>
#include <stdexcept>
// TCP port --------------------
// lecture 6 page 29 ("echo server with stream sockets"; for some version page 25/26)
#include <unistd.h>
#include <arpa/inet.h>
// threads
#include <pthread.h>
// signal handler
#include <signal.h>
#include <vector>
#include <algorithm> //FIND command for VECTOR class
// open directory
#include <dirent.h> //http://man7.org/linux/man-pages/man3/opendir.3.html
#include <string>
#include <ctime>
#include <fstream>
#include <sys/stat.h>


#include "EmailRepositoryClient.h"
#include "httpservice.h"	
#define LEN_MAX_CMD (1000)
#define MAX_LEN_MESS (LEN_MAX_CMD*LEN_MAX_CMD)
#define MAX_connection (100) // handout -- assume that there will be no more than 100 concurrent connections (however, your server should not limit the number of sequential connnections)
#define MAX_LEN_PARENTdir (1000)
#define MAX_USER_NAME (256)
#define STATE_INITI (0)
#define STATE_HELO_MAIL (1)
#define STATE_MAIL_RCPT (2)
#define STATE_RCPT_DATA (3)
#define STATE_DATA (4)

using namespace std;
// // global variables
// vector<int> Client_fds;
// //vector<FILE *> Read_Write_fds;
// vector<pthread_t> Threads; 
// // vector<BigTable *> BIGTABLE_pts; // something wrong with string
// char PARENTdir[MAX_LEN_PARENTdir];
int SMTP_external_to_bigtables(int connfd, bool db, MasterServiceClient* masterServer); // main function
#endif