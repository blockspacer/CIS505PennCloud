/*
 * LoadBalancer.cc
 *
 *  Created on: Nov 17, 2017
 *      Author: cis505
 */

#include "../common/json.hpp"
#include "../FE_server/feconfig.h"
#include "../FE_server/feserver.h"
#include "../FE_server/httpservice.h"

using json = nlohmann::json;

//============================
// system funcs
//============================
static void http_sig_handler(int signum){
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

	if (!configFile.is_open()) {
		cerr << "Could not open config file '" << configFileName << "'" << endl;
		exit(1);
	}

	vector<ServerStatusDTO> servers;
	while(getline(configFile, line)) {
		vector<string> tokens = stringSplit(line, ',');

		if (tokens.size() == 4) {
			int groupId = stoi(tokens[0]);
			string address = tokens[1];
			int port = stoi(tokens[2]);
			string heartbeatAddress = tokens[3];

			ServerStatusDTO server;
			server.set_groupid(groupId);
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

json serverStatusDtoToJson(const ServerStatusDTO &serverStatus) {

	string status = "RUNNING";
	if (serverStatus.status() == ServerStatus::RECOVERING) {
		status = "RECOVERING";
	}
	else if (serverStatus.status() == ServerStatus::UNKNOWN) {
		status = "UNKNOWN";
	}

	json jsonObj;
	jsonObj["address"] = serverStatus.ipaddress() + ":" + to_string(serverStatus.port());
	jsonObj["status"] = status;
	jsonObj["heartbeatAddress"] = serverStatus.heartbeataddress();

	return jsonObj;
}

string serverStatusVectorToJson(const vector<ServerStatusDTO> &serverStatuses) {

	vector<json> jsonVector;

	for (const ServerStatusDTO &server : serverStatuses) {
		jsonVector.push_back(serverStatusDtoToJson(server));
	}

	json jsonObj = json(jsonVector);
	return jsonObj.dump();
}

void writeServerStatusToFile(const vector<ServerStatusDTO> &serverStatuses, string fileName) {
	ofstream file("./views/" + fileName);

	if (file.is_open()) {
		file << serverStatusVectorToJson(serverStatuses);
	}
	else {
		cerr << "Could not write server status file " << fileName << endl;
	}

	file.close();
}

void heartbeatThreadFn(AdminConsoleData* adminConsoleData) {
	setmask();
	cerr << "Starting heartbeat thread" << endl;

	// Stop when the run flag is set to false
	while (adminConsoleData->run) {
		for (int i = 0; i < adminConsoleData->feServers.size(); i++) {
			ServerStatusDTO* server = &(adminConsoleData->feServers[i]);

			cerr << "Checking heartbeat of FE server" << server->ipaddress() << ":" << server->port() << endl;

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
			cerr << "Heartbeat response of FE server " << server->ipaddress() << ":" << server->port() << ":" << statusStr << endl;
			lock_guard<mutex> lock(adminConsoleData->mtx);
			server->set_status(newStatus.status());
		}

		for (int i = 0; i < adminConsoleData->beServers.size(); i++) {
			ServerStatusDTO* server = &(adminConsoleData->beServers[i]);

			cerr << "Checking heartbeat of BE server " << server->ipaddress() << ":" << server->port() << endl;

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
			cerr << "Heartbeat response of BE server " << server->ipaddress() << ":" << server->port() << ":" << statusStr << endl;
			lock_guard<mutex> lock(adminConsoleData->mtx);
			server->set_status(newStatus.status());
		}
		writeServerStatusToFile(adminConsoleData->feServers, "feServerData");
		writeServerStatusToFile(adminConsoleData->beServers, "beServerData");

		this_thread::sleep_for(chrono::seconds(10));
	}
}

int main(int argc, char *argv[]){
    int c = 0;
    int pflag = 0, aflag = 0, vflag = 0;
    char * pvalue = NULL;
    int server_port = 7000;  // default port = 7000
    string masterAddr = "localhost:50080";

    //parse input flag using getopt()
    while((c = getopt(argc, argv, "p:m:av"))!= -1){
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
            case 'm':
            	masterAddr = optarg;
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

    string feConfigFile;
    string beConfigFile;
    if (argc - optind < 2) {
    	fprintf(stderr, "Usage: [-v] [-p <port>] <feServerConfigFile> <beServerConfigFile>\n");
    	exit(1);
    }
    else {
    	feConfigFile = std::string(argv[optind]);
    	beConfigFile = std::string(argv[optind + 1]);
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

    // TODO: This should be changed to the big table master server address eventually
    grpc::ChannelArguments channelArgs;
    channelArgs.SetMaxReceiveMessageSize(-1);
    AdminConsoleRepositoryClient adminClient(grpc::CreateCustomChannel("localhost:50051", grpc::InsecureChannelCredentials(), channelArgs));
    MasterServiceClient masterServer(grpc::CreateChannel(masterAddr, grpc::InsecureChannelCredentials()));

    AdminConsoleData data;
    data.feServers = parseConfigFile(feConfigFile);
    data.beServers = parseConfigFile(beConfigFile);
    data.adminConsoleRepository = &adminClient;
    data.masterServer = &masterServer;

    // Set up a thread to periodically send a heartbeat message to each FE & BE server
    thread heartbeatThread(heartbeatThreadFn, &data);

    // default port and host: localhost:7000
    Fs.runAdminConsole(data);

    // Let the heartbeat thread know to stop
    data.run = false;
    heartbeatThread.join();

    return 0;
}

