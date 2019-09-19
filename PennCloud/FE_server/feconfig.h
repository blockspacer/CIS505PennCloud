/*
 * feserverconfig.h
 * Front End global config file
 * PennCloud CIS 505
 * Author T01
 * Student ID : 31295008
 * Date: Nov 6 201
 */

#ifndef _FECONFIG_H
#define _FECONFIG_H
//-- c, system and pthread
#include <unistd.h>
#include <stdlib.h>
#include <errno.h> 
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <thread>
#include <future>
#include <signal.h>
//-- c++
#include <string.h>
#include <iostream>
#include <string>  
#include <cstring>  
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>   
#include <unordered_map>
#include <queue>
//-- socket programming
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//-- openssl
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
//-- grpc and protoc
#include <grpc++/grpc++.h>
#include "EmailRepositoryClient.h"
#include "UserRepositoryClient.h"
#include "StorageRepositoryClient.h"
#include "../common/HeartbeatServiceClient.h"
#include "../Admin_Console/AdminConsoleRepositoryClient.h"
#include <arpa/nameser.h>

using namespace std;

//const BIO_METHOD *BIO_f_base64(void);
//---------- FEEL free to modify the consts..A lot of the number are just me made them up....
const int Max_usr_name = 64;
const int Max_domain_name = 64;
const int Max_addr = 256;
const int Max_path = 256;
const long long Max_cmd = 5120;
const int Max_rly = 512;
const int Max_textl = 1000;
const int Max_recipient= 100;
const int Max_conn_capacity = 100;
const int local_port = 8888;

const string local_host = "localhost";
const string http_cmd_term = "\r\n\r\n";
const string Http10 = "HTTP/1.0";
const string Http11 = "HTTP/1.1";
const string Get = "GET";
const string Post = "POST";
const string Put = "PUT";
const string Head = "HEAD";
const string Delete = "DELETE";

const string OK = "200 OK\r\n";
const string Notfound = "404 Not Found\r\n";
const string NotImp = "501 Not Implemented\r\n";

const string ctype_plain = "text/plain";
const string ctype_html = "text/html";
const string ctype_js = "text/javascript";
const string ctype_css = "text/css";
const string ctype_xml = "text/xml"; 
const string ctype_jpg = "image/jpg"; 
const string ctype_png = "image/png"; 
const string ctype_jpeg = "image/jpeg"; 

static bool db = false;

static unordered_set<pthread_t> threads;

static promise<void> exit_requested;

//system functions
static void http_sig_handler(int signum);
static void setmask();
static void interrupt(int connfd);
static void *worker(void *arg);
static void *loadBalancer(void *arg);
static void *adminConsoleWorker(void *arg);

#endif //_FECONFIG_H
