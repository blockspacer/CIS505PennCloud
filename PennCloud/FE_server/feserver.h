/*
 * feserver.h
 * PennCloud CIS 505
 * Author T01
 * Date: Nov 8 2017
 */

#pragma once
#include <mutex>
#include "feconfig.h"
#include "httpservice.h"
#include "../Admin_Console/AdminConsoleRepositoryClient.h"
#include "../common/HeartbeatServiceClient.h"
#include "../common/MasterServiceClient.h"

using heartbeatservice::ServerStatusDTO;

using namespace std;

typedef struct {
	volatile bool run = true;
	mutex mtx;
	vector<ServerStatusDTO> feServers;
} LoadBalancerData;

typedef struct {
	volatile bool run = true;
	mutex mtx;
	AdminConsoleRepositoryClient* adminConsoleRepository;
	MasterServiceClient* masterServer;
	vector<ServerStatusDTO> feServers;
	vector<ServerStatusDTO> beServers;
} AdminConsoleData;

class FEServer{
private:
    int port;
    string host;
    pthread_t mainThread;

public:
    FEServer(const string &host, int port);
    ~FEServer();
    int run(MasterServiceClient &masterServer);
    int runLoadBalancer(LoadBalancerData &loadBalancerData);
    int runAdminConsole(AdminConsoleData &adminConsoleData);
    int interruptFeServer() {
    	return pthread_kill(mainThread, SIGINT);
    }
};



