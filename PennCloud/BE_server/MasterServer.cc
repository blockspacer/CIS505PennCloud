/*
 * MasterServer.cc
 *
 *  Created on: Nov 29, 2017
 *      Author: cis505
 */

#include "MasterServer.h"

vector<string> stringSplit(const string &input, const char &delimiter) {
	stringstream stringStream(input);
	string token;
	vector<string> tokens;

	while(getline(stringStream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
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

void heartbeatThreadFn(BeServerData* serverData) {
	setmask();
	cerr << "Starting heartbeat thread" << endl;

	// Stop when the run flag is set to false
	while (serverData->run) {
		for (auto& entry : serverData->serverGroupMap) {
			deque<ServerStatusDTO>* serverGroup = &(entry.second);

			bool primaryFailed = false;
			for (int i = 0; i < serverGroup->size(); i++) {

				ServerStatusDTO* server = &(serverGroup->at(i));
				cerr << "Checking heartbeat of " << server->ipaddress() << ":" << server->port()
						<< " (Group: " << entry.first << ", i: " << i << ")" << endl;

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
				lock_guard<mutex> lock(serverData->mtx);
				ServerStatus oldStatus = server->status();

				if (newStatus.status() == ServerStatus::UNKNOWN && oldStatus != ServerStatus::UNKNOWN && i == 0) {
					primaryFailed = true;
				}
				// Update the server's status
				server->set_status(newStatus.status());
			}

			if (primaryFailed) {
				// If the primary server goes down (the first server is treated as the primary) then move that server to the back of the deque
				ServerStatusDTO oldPrimary = serverGroup->at(0);
				serverGroup->pop_front();
				serverGroup->push_back(oldPrimary);
				cerr << "Primary server failed for group " << entry.first << ". Designating a new primary" << endl;
			}
		}

		this_thread::sleep_for(chrono::seconds(10));
	}
}

map<int, deque<ServerStatusDTO>> parseConfigFile(const string &configFileName) {
	std::ifstream configFile(configFileName);
	std::string line;

	map<int, deque<ServerStatusDTO>> servers;
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

			servers[groupId].push_back(server);
		}
		else {
			cerr << "Invalid config file" << endl;
			exit(1);
		}
	}

	return servers;
}

void Run_Server(const string &serverAddress, BeServerData* serverDataPtr) {
	string directory = "server1";
	vector<string> tokens = stringSplit(serverAddress, ':');
	string address = tokens[0];
	int port = stoi(tokens[1]);

    MasterServiceImpl masterService(serverDataPtr);
    ServerToMasterImpl server2MasterService(serverDataPtr);

	ServerBuilder builder;
	builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
	builder.RegisterService(&masterService);
	builder.RegisterService(&server2MasterService);

	builder.SetMaxMessageSize(20 * 1024 * 1024);

	unique_ptr<Server> server(builder.BuildAndStart());
	cout << "Server listening on " << serverAddress << endl;
	server->Wait();
}


int main(int argc, char *argv[]) {
	int c = 0;
	bool vflag = false;
	string address = "0.0.0.0:50080";

	//parse input flag using getopt()
	while((c = getopt(argc, argv, "a:v"))!= -1){
		switch(c){
		case 'a': {
			string arg(optarg);
			vector<string> tokens = stringSplit(arg, ':');
			if (tokens.size() != 2) {
				cerr << "Address argument should be of form <ipaddress>:<port>" << endl;
				exit(1);
			}
			else {
				address = arg;
			}
			break;
		}
		case 'v':
			vflag = true;
			break;
		case '?':
			if (optopt == 'a') {
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			}
			else {
				fprintf(stderr, "Unknown option '-%c.\n",optopt);
			}
			exit(1);
		default:
			cout << "Usage: [-v] [-a <ipaddress>:<port>]" << endl;
			abort();
		}
	}

	string configFile;
	if (argc - optind < 1) {
		fprintf(stderr, "Usage: [-v] [-p <port>] <serverConfigFile>\n");
		exit(1);
	}
	else {
		configFile = std::string(argv[optind]);
	}

	BeServerData serverData;
	serverData.serverGroupMap = parseConfigFile(configFile);

	// Initialize the sequence number for each server group
	for (auto &entry : serverData.serverGroupMap) {
		int seqNum = 0;
		serverData.seqNumMap.emplace(entry.first, seqNum);
		serverData.loadMap.emplace(entry.first, 0);
	}

	// Set up a thread to periodically send a heartbeat message to each BE server
	thread heartbeatThread(heartbeatThreadFn, &serverData);

	Run_Server(address, &serverData);

	return 0;
}

