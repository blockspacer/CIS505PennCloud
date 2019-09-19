/*
 * LoadBalancer.cc
 *
 *  Created on: Nov 17, 2017
 *      Author: cis505
 */

#include <thread>

#include "../FE_server/feconfig.h"
#include "../FE_server/feserver.h"
#include "../FE_server/httpservice.h"
#include "../common/HeartbeatServiceClient.h"

using heartbeatservice::ServerStatusDTO;

//============================
// system funcs
//============================
static void http_sig_handler(int signum){
    //int hasworker = 0;
    switch(signum){
        case SIGINT:
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

void heartbeatThreadFn(LoadBalancerData* loadBalancerData) {
	setmask();
	cerr << "Starting heartbeat thread" << endl;

	// Stop when the run flag is set to false
	while (loadBalancerData->run) {
		for (int i = 0; i < loadBalancerData->feServers.size(); i++) {
			ServerStatusDTO* server = &(loadBalancerData->feServers[i]);

			cerr << "Checking heartbeat of " << server->ipaddress() << ":" << server->port() << endl;

			// TODO: Update to not create a new client each time. I was having trouble storing multiple clients in a vector and accessing them here
			HeartbeatServiceClient heartbeatClient(grpc::CreateChannel(server->heartbeataddress(), grpc::InsecureChannelCredentials()));
			ServerStatusDTO newStatus = heartbeatClient.heartbeat(server->ipaddress(), server->port());

			string statusStr;
			if (newStatus.status() == ServerStatus::RUNNING) {
				statusStr = "RUNNING";
			}
			else {
				statusStr = "UNKNOWN";
			}
			cerr << "Heartbeat response of " << server->ipaddress() << ":" << server->port() << ":" << statusStr << endl;
			lock_guard<mutex> lock(loadBalancerData->mtx);
			server->set_status(newStatus.status());
		}

		this_thread::sleep_for(chrono::seconds(10));
	}
}

vector<string> stringSplit(const string &input, const char &delimiter) {
	stringstream stringStream(input);
	string token;
	vector<string> tokens;

	while(getline(stringStream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

vector<ServerStatusDTO> parseConfigFile(const string &configFileName) {
	std::ifstream configFile(configFileName);
	std::string line;

	vector<ServerStatusDTO> servers;
	while(getline(configFile, line)) {
		vector<string> tokens = stringSplit(line, ',');

		if (tokens.size() == 3) {
			string address = tokens[0];
			int port = stoi(tokens[1]);
			string heartbeatAddress = tokens[2];

			ServerStatusDTO server;
			server.set_ipaddress(address);
			server.set_port(port);
			server.set_heartbeataddress(heartbeatAddress);
			server.set_status(ServerStatus::UNKNOWN);

			servers.push_back(server);
		}
		else {
			cerr << "Invalid config file" << endl;
			exit(1);
		}
	}

	return servers;
}

int main(int argc, char *argv[]){
    int c = 0;
    int pflag = 0, aflag = 0, vflag = 0;
    char * pvalue = NULL;
    int server_port = 8000;  // default port = 8888

    //parse input flag using getopt()
    while((c = getopt(argc, argv, "p:av"))!= -1){
        switch(c){
            case 'p':
                pvalue = optarg;
                pflag = 1;
                break;
            case 'a':
                aflag = 1;
                break;
            case 'v':
                vflag = 1;
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
    if (aflag == 1) {
        fprintf(stderr, "YudingAi, yudingai\n");
        exit(1);
    }

    if (vflag == 1) {
        db = true;
    }

    if (pflag == 1) {
        server_port = stoi(pvalue);
    }

    string configFile;
    if (argc - optind < 1) {
    	fprintf(stderr, "Usage: [-v] [-p <port>] <serverConfigFile>\n");
    	exit(1);
    }
    else {
    	configFile = std::string(argv[optind]);
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

    LoadBalancerData lbData;
    lbData.feServers = parseConfigFile(configFile);

    // Set up a thread to periodically send a heartbeat message to each FE server
    thread heartbeatThread(heartbeatThreadFn, &lbData);

    // default port and host: localhost:8000
    Fs.runLoadBalancer(lbData);

    // Let the heartbeat thread know to stop
    lbData.run = false;
    heartbeatThread.join();

    return 0;
}

