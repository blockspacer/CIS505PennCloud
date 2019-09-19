/*
 * feserverMain.cc
 *
 *  Created on: Nov 17, 2017
 *      Author: cis505
 */
#include "../common/HeartbeatServiceImpl.h"
#include "feserver.h"

//============================
// system funcs
//============================
static void http_sig_handler(int signum){
    //int hasworker = 0;
    switch(signum){
        case SIGINT:
        	exit_requested.set_value();
            if(threads.size() == 0){
                break;
            }
            for(pthread_t i : threads){
                pthread_kill(i, SIGALRM);
            }
            break;
        case SIGALRM:
            break;
        default:
            break;
    }
}

//===========
//main
//===========
int main(int argc, char *argv[]){
    int c = 0;
    int pflag = 0, aflag = 0, vflag = 0;
    int mflag = 0;
    char * pvalue = NULL;
    char * mvalue = NULL;
    int server_port = local_port;  // default port = 8888
    string master_address = "localhost:50080";
    int hb_port = 50070;

    //parse input flag using getopt()
    while((c = getopt(argc, argv, "m:p:h:av"))!= -1){
        switch(c){
            case 'p':
                pvalue = optarg;
                pflag = 1;
                break;
            case 'h':
            	hb_port = stoi(optarg);
            	break;
            case 'a':
                aflag = 1;
                break;
            case 'v':
                vflag = 1;
                break;
            case 'm':
                mvalue = optarg;
                mflag = 1;
                break;
            case '?':
                if(optopt == 'p'){
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                }else {
                    fprintf(stderr, "Unknown option '-%c.\n",optopt);
                }
                exit(1);
            default:
                abort();
        }
    }
    if(aflag == 1){
        fprintf(stderr, "YudingAi, yudingai\n");
        exit(1);
    }
    if(vflag == 1){
        db = true;
    }
    //cerr << "from main db value is "<<db << endl;
    if(pflag == 1){
        server_port = stoi(pvalue);
    }
    if(mflag == 1){
        master_address = mvalue;
    }

    srand(time(NULL));
    FEServer Fs(local_host, server_port);
    struct sigaction sa;
    sa.sa_handler = http_sig_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if(sigaction(SIGINT, &sa, NULL) == -1){
        perror("SIGACTION");
    }
    if(sigaction(SIGALRM, &sa, NULL) == -1){
        perror("SIGACTION");
    }

//    MasterServiceClient masterServer(grpc::CreateChannel("localhost:50080", grpc::InsecureChannelCredentials()));
    MasterServiceClient masterServer(grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials()));

    // Set up a heartbeat server
    ServerStatus status = ServerStatus::RUNNING;
    HeartbeatServiceImpl heartbeatServer(local_host, server_port, &status);

    // TODO: make this address configurable
    unique_ptr<Server> server = heartbeatServer.runHeartbeatServer("localhost:" + to_string(hb_port));

    auto shutDownFn = [&]() {
    	auto future = exit_requested.get_future();
    	future.wait();
    	cerr << "Shutting down heartbeat server" << endl;
    	server->Shutdown();
    	cerr << "Interrupting FE Server" << endl;
    	exit_requested = promise<void>();
    	Fs.interruptFeServer();
    };

    thread shutdownThread(shutDownFn);

    // default port and host: localhost:8888
    Fs.run(masterServer);
    shutdownThread.join();
    return 0;
}

