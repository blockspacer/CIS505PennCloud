#include <algorithm>
#include <arpa/inet.h>
#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using namespace std;

// wrapper class for client.
class Client {
public:
	sockaddr_in address;
	int room;
	string nick_name;

public:
	Client(sockaddr_in address): address(address), room(0) {};
};

// wrapper class for a message's properties.
class Message {
public:
	int id;
	int sender;
	bool deliverable;
	vector< int > clock;

public:
	Message(): id(0), sender(0), deliverable(false) {};
	Message(int id, int sender): id(id), sender(sender), deliverable(false) {};
	// parse a vector clock string into a vector of integer.
	Message(int sender, char* clock) {
		this->id = 0;
		this->sender = sender;
		this->deliverable = true;

		char* c = strtok(clock, "~");
		this->clock.push_back(atoi(c));
		while ((c = strtok(NULL, "~")) != NULL) {
			this->clock.push_back(atoi(c));
		}
	}
};

// compare function for holdback queue.
struct MessageCompare {
	bool operator()(const Message& m1, const Message& m2) const {
		// tie breaker
		if (m1.id < m2.id || (m1.id == m2.id && m1.sender < m2.sender)) {
			return true;
		} else {
			return false;
		}
	}
};

// constants
const int MSG_LEN = 1024;
const int UNORDERED = 0;
const int FIFO = 1;
const int CAUSAL = 2;
const int TOTAL = 3;
// maximum number of chat rooms can be changed here
const int NUM_OF_ROOMS = 10;
const char NEW_MESSAGE = 0;
const char PROPOSAL = 1;
const char AGREEMENT = 2;
const char* JOIN_FIRST 		= "-ERR You need to join a room.";
const char* JOIN_OK			= "+OK You are now in room #";
const char* JOIN_ERR		= "-ERR You are already in room #";
const char* PART_OK			= "+OK You have left room #";
const char* UNKNOWN_COMMAND = "-ERR Unknown command.";

// global variables
vector< sockaddr_in > SERVERS;
vector< Client > CLIENTS;
vector< vector< unordered_map< int, string > > > HOLD_BACK_QUEUE_FIFO;
vector< vector< Message > > HOLD_BACK_QUEUE_CAUSAL;
vector< map< Message, string, MessageCompare > > HOLD_BACK_QUEUE_TOTAL;
vector< vector< int > > RECEIVED;
vector< int > FIFO_MESSAGE_ID;
vector< int > CLOCK;
vector< int > PROPOSED;
vector< int > AGREED;
bool DEBUG = false;
int SELF_INDEX = 0;
int ORDER = UNORDERED;

// function declarations
sockaddr_in string_to_sockaddr(char* address);
bool is_server(sockaddr_in src, int* index);
bool is_client(sockaddr_in src, int* index);
void handle_new_client(Client* c, char* buffer, int listen_fd);
void handle_client_message(int client_index, char* buffer, int listen_fd, sockaddr_in self);
char* add_name(int listen_fd, char* buffer, Client client);
void forward_to_clients(int listen_fd, char* buffer, int room);
void forward_to_servers(int listen_fd, char* buffer, bool all, sockaddr_in self);
void handle_fifo_order(int listen_fd, int sender, int msg_id, int room, char* message);
void handle_causal_order(int listen_fd, int sender, char* clock, int room, char* message);
void handle_total_order(int listen_fd, int sender, int type, int proposed_by, int msg_id, int room, char* message, unordered_map< string, vector< Message > > &proposals);
bool have_seen_all(Message& msg, int sender);
string debug_prefix();

// main function. Read command line options. Read server address file. Call message handlers in main loop.
int main(int argc, char *argv[]) {
	int option = 0;
	// parse command line arguments
	while ((option = getopt(argc, argv, "vo:")) != -1) {
		switch (option) {
			case 'v':
				DEBUG = true;
				break;
			case 'o':
				if (strcasecmp(optarg, "unordered") == 0) {
					ORDER = UNORDERED;
				} else if (strcasecmp(optarg, "fifo") == 0) {
					ORDER = FIFO;
				} else if (strcasecmp(optarg, "causal") == 0) {
					ORDER = CAUSAL;
				} else if (strcasecmp(optarg, "total") == 0) {
					ORDER = TOTAL;
				} else {
					cerr << "Please enter a valid ordering" << endl;
					exit(1);
				}
				break;
			default:
				cerr << "Usage: " << argv[0] << " <configuration> <index> [-v] [-o ordering]" << endl;
				exit(2);
		}
	}
	if (optind == argc) {
		cerr << "*** Author: Han Zhu (zhuhan)" << endl;
		exit(3);
	}

	string file_name = argv[optind];
	ifstream config_file(file_name);

	optind++;
	if (optind == argc) {
		cerr << "Please enter server index" << endl;
		exit(4);
	}
	SELF_INDEX = atoi(argv[optind]);

	// read server configuration file.
	int i = 0;
	struct sockaddr_in self;
	string line;
	while (getline(config_file, line)) {
		char address[line.length() + 1];
		strcpy(address, line.c_str());

		char* forward = strtok(address, ",");
		char* bind = strtok(NULL, ",");	
		SERVERS.push_back(string_to_sockaddr(forward));
		if (i == SELF_INDEX - 1) {
			if (bind != NULL) {
				self = string_to_sockaddr(bind);
			} else {
				self = SERVERS[i];
			}
		}
		i++;
	}

	// bind this server to its bind address.
	self.sin_addr.s_addr = htons(INADDR_ANY);
	int listen_fd = socket(PF_INET, SOCK_DGRAM, 0);
	bind(listen_fd, (struct sockaddr*) &self, sizeof(self));

	// initialize global variables
	for (int i = 0; i < NUM_OF_ROOMS; i++) {
		FIFO_MESSAGE_ID.push_back(0);
		vector< unordered_map< int, string > > v1;
		HOLD_BACK_QUEUE_FIFO.push_back(v1);
		vector< Message > s;
		HOLD_BACK_QUEUE_CAUSAL.push_back(s);
		map< Message, string, MessageCompare > m;
		HOLD_BACK_QUEUE_TOTAL.push_back(m);
		vector< int > v2;
		RECEIVED.push_back(v2);
		PROPOSED.push_back(0);
		AGREED.push_back(0);

		for (int j = 0; j < SERVERS.size(); j++) {
			unordered_map< int, string > map;
			HOLD_BACK_QUEUE_FIFO[i].push_back(map);
			RECEIVED[i].push_back(0);
		}
	}

	for (int i = 0; i < SERVERS.size(); i++) {
		CLOCK.push_back(0);
	}

	// this variable will be used in handle_total_order().
	unordered_map< string, vector< Message > > proposals;

	// main loop
	while (true) {
		// receive from socket
		struct sockaddr_in src;
		socklen_t src_len = sizeof(src);
		char buffer[MSG_LEN];
		int len = recvfrom(listen_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*) &src, &src_len);
		buffer[len] = 0;

		int index = 0;
		// if the message is from a server
		if (is_server(src, &index)) {
			if (DEBUG) {
				cout << debug_prefix() << " Server " << index << " sends \"" << buffer << "\"" << endl;
			}
			
			// parse a server's message in the format of "type(total ordering), proposed_by, sequence number, chat room, message content".
			char* clock = strtok(buffer, ",");
			char* type = strtok(NULL, ",");
			char* proposed_by = strtok(NULL, ",");
			char* msg_id = strtok(NULL, ",");
			char* room = strtok(NULL, ",");
			char* message = strtok(NULL, ",");

			// handle a server's message according to the ordering.
			if (ORDER == UNORDERED) {
				forward_to_clients(listen_fd, message, atoi(room));
			} else if (ORDER == FIFO) {
				handle_fifo_order(listen_fd, index, atoi(msg_id), atoi(room), message);
			} else if (ORDER == CAUSAL) {
				handle_causal_order(listen_fd, index, clock, atoi(room), message);
			} else {
				handle_total_order(listen_fd, index, atoi(type), atoi(proposed_by), atoi(msg_id), atoi(room), message, proposals);
			}

		// if the message is from an existing client
		} else if (is_client(src, &index)) {
			if (DEBUG) {
				cout << debug_prefix() << " Client " << index << " posts \"" << buffer << "\""<< endl;
			}
			handle_client_message(index, buffer, listen_fd, self);

		// if the message is from a new client
		} else {
			// add the client to CLIENTS.
			Client c(src);
			CLIENTS.push_back(c);
			index = CLIENTS.size();

			if (DEBUG) {
				cout << debug_prefix() << " Client " << index << " posts \"" << buffer << "\""<< endl;
			}

			handle_new_client(&CLIENTS[index - 1], buffer, listen_fd);
		}
	}

	return 0;
}

// converts an ip address parsed from the configuration file into a sockaddr_in struct.
// address: ip address from the server configuration file.
// return:	a sockaddr_in struct.
sockaddr_in string_to_sockaddr(char* address) {
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;

	char* token = strtok(address, ":");
	inet_pton(AF_INET, token, &addr.sin_addr);

	token = strtok(NULL, ":");
	addr.sin_port = htons(atoi(token));

	return addr;
}

// check if the source of the message is a server.
// src:		message source.
// index:	if src is a server, index will be changed to the server's index.
// return:	true if src is a server, false otherwise.
bool is_server(sockaddr_in src, int* index) {
	for (int i = 0; i < SERVERS.size(); i++) {
		sockaddr_in server = SERVERS[i];
		// compare ip and port
		if (strcmp(inet_ntoa(server.sin_addr), inet_ntoa(src.sin_addr)) == 0 && server.sin_port == src.sin_port) {
			*index = i + 1;
			return true;
		}
	}

	return false;
}

// check if the source of the message is a client.
// src:		message source.
// index:	if src is a client, index will be changed to the client's index in CLIENTS.
// return:	true if src is a client, false otherwise.
bool is_client(sockaddr_in src, int* index) {
	for (int i = 0; i < CLIENTS.size(); i++) {
		// compare ip and port
		if (strcmp(inet_ntoa(CLIENTS[i].address.sin_addr), inet_ntoa(src.sin_addr)) == 0 && CLIENTS[i].address.sin_port == src.sin_port) {
			*index = i + 1;
			return true;
		}
	}

	return false;
}

// handler for a new client. A new client's valid commands are only /join and /nick.
// c:			pointer to the client stored in the global vector.
// buffer:		message the client sent.
// listen_fd:	socket that this server is listening to.
void handle_new_client(Client* c, char* buffer, int listen_fd) {
	char* token = strtok(buffer, " ");
	char message[MSG_LEN];
	
	if (strcasecmp(token, "/nick") == 0) {
		char name[16];
		token = strtok(NULL, " ");
		strcpy(name, token);
		while ((token = strtok(NULL, " ")) != NULL) {
			strcat(name, " ");
			strcat(name, token);
		}
		c->nick_name = string(name);
		sprintf(message, "+OK Nick name set to \"%s\"", name);
		sendto(listen_fd, message, MSG_LEN, 0, (struct sockaddr*) &c->address, sizeof(c->address));

	// if the client didn't sent /nick or /join
	} else if (strcasecmp(token, "/join") != 0) {
		sendto(listen_fd, JOIN_FIRST, strlen(JOIN_FIRST), 0, (struct sockaddr*) &c->address, sizeof(c->address));

	// /join
	} else {
		token = strtok(NULL, " ");
		int room = atoi(token);
		if (room > NUM_OF_ROOMS) {
			sprintf(message, "-ERR There are only %d chat rooms.", NUM_OF_ROOMS);
			sendto(listen_fd, message, MSG_LEN, 0, (struct sockaddr*) &c->address, sizeof(c->address));
		} else {
			sprintf(message, "%s%d", JOIN_OK, room);
			c->room = room;
			sendto(listen_fd, message, strlen(message), 0, (struct sockaddr*) &c->address, sizeof(c->address));
		}
	}
}

// handler for a client's message.
// client_index:	index of a client in CLIENTS.
// buffer:			message received.
// listen_fd:		socket this server is listening to.
// self:			this server as a sockaddr_in struct.
void handle_client_message(int client_index, char* buffer, int listen_fd, sockaddr_in self) {
	Client* c = &CLIENTS[client_index - 1];

	// if client sent a command
	if (buffer[0] == '/') {
		char* arg = strtok(buffer, " ");
		char message[MSG_LEN];

		if (strcasecmp(arg, "/join") == 0) {
			// if client is already in a room
			if (c->room != 0) {
				sprintf(message, "%s%d", JOIN_ERR, c->room);
				sendto(listen_fd, message, strlen(message), 0, (struct sockaddr*) &c->address, sizeof(c->address));
			} else {
				arg = strtok(NULL, " ");
				int room = atoi(arg);
				if (room > NUM_OF_ROOMS) {
					sprintf(message, "-ERR There are only %d chat rooms.", NUM_OF_ROOMS);
					sendto(listen_fd, message, MSG_LEN, 0, (struct sockaddr*) &c->address, sizeof(c->address));
				} else {
					sprintf(message, "%s%d", JOIN_OK, room);
					c->room = room;
					sendto(listen_fd, message, strlen(message), 0, (struct sockaddr*) &c->address, sizeof(c->address));
				}
			}

		} else if (strcasecmp(arg, "/part") == 0) {
			// if client is not in any room.
			if (c->room == 0) {
				sendto(listen_fd, JOIN_FIRST, strlen(JOIN_FIRST), 0, (struct sockaddr*) &c->address, sizeof(c->address));				
			} else {
				sprintf(message, "%s%d", PART_OK, c->room);
				c->room = 0;
				sendto(listen_fd, message, MSG_LEN, 0, (struct sockaddr*) &c->address, sizeof(c->address));
			}

		} else if (strcasecmp(arg, "/nick") == 0) {
			char name[16];
			// get the whole name if it has spaces.
			arg = strtok(NULL, " ");
			strcpy(name, arg);
			while ((arg = strtok(NULL, " ")) != NULL) {
				strcat(name, " ");
				strcat(name, arg);
			}
			c->nick_name = string(name);
			sprintf(message, "+OK Nick name set to \"%s\"", name);
			sendto(listen_fd, message, MSG_LEN, 0, (struct sockaddr*) &c->address, sizeof(c->address));

		} else if (strcasecmp(arg, "/quit") == 0) {
			// delete client from CLIENTS
			CLIENTS.erase(CLIENTS.begin() + client_index - 1);

		} else {
			sendto(listen_fd, UNKNOWN_COMMAND, strlen(UNKNOWN_COMMAND), 0, (struct sockaddr*) &c->address, sizeof(c->address));
		}

	// if client sent a real message
	} else {
		if (c->room == 0) {
			sendto(listen_fd, JOIN_FIRST, strlen(JOIN_FIRST), 0, (struct sockaddr*) &c->address, sizeof(c->address));
		} else {
			char* content = add_name(listen_fd, buffer, *c);

			char message[MSG_LEN];
			// causal ordering requires the vector clock be attached in the message.
			if (ORDER == CAUSAL) {
				forward_to_clients(listen_fd, content, c->room);
				CLOCK[SELF_INDEX - 1]++;

				string vc = to_string(CLOCK[0]);
				for (int i = 1; i < CLOCK.size(); i++) {
					vc += "~" + to_string(CLOCK[i]);
				}

				sprintf(message, "%s,%d,%d,%d,%d,%s", vc.c_str(), NEW_MESSAGE, 0, 0, c->room, content);
				forward_to_servers(listen_fd, message, false, self);

			// total ordering requires a different sequence number from fifo.
			} else if (ORDER == TOTAL) {
				sprintf(message, "%s,%d,%d,%d,%d,%s", "n/a", NEW_MESSAGE, 0, 0, c->room, content);
				forward_to_servers(listen_fd, message, true, self);

			// unordered doesn't care about the sequence number, so the message can be the same as fifo.
			} else {
				forward_to_clients(listen_fd, content, c->room);
				FIFO_MESSAGE_ID[c->room - 1]++;
				sprintf(message, "%s,%d,%d,%d,%d,%s", "n/a", NEW_MESSAGE, 0, FIFO_MESSAGE_ID[c->room - 1], c->room, content);
				forward_to_servers(listen_fd, message, false, self);
			}
			free(content);
		}
	}
}

// add a client's name to its message before sending.
// listen_fd:	socket this server is listening to.
// buffer:		message without client's name.
// client:		client that sent the message.
// return:		message with client's name inserted in the beginning.
char* add_name(int listen_fd, char* buffer, Client client) {
	char* message = (char*) malloc(sizeof(char) * MSG_LEN);

	if (client.nick_name.empty()) {
		char name[MSG_LEN];
		sprintf(name, "%s:%d", inet_ntoa(client.address.sin_addr), client.address.sin_port);
		sprintf(message, "<%s> %s", name, buffer);
	} else {
		sprintf(message, "<%s> %s", client.nick_name.c_str(), buffer);
	}

	return message;
}

// multicast to all clients that are known to this server
// listen_fd:	socket this server is listening to.
// buffer:		message to be sent.
// room:		chat room where the message will be sent.
void forward_to_clients(int listen_fd, char* buffer, int room) {
	for (int i = 0; i < CLIENTS.size(); i++) {
		if (CLIENTS[i].room == room) {
			sendto(listen_fd, buffer, strlen(buffer), 0, (struct sockaddr*) &CLIENTS[i], sizeof(CLIENTS[i]));

			if (DEBUG) {
				cout << debug_prefix() << " Sent \"" << buffer << "\" to client " << i + 1 << " at room #" << CLIENTS[i].room << endl;
			}
		}
	}
}

// multicast to all servers.
// listen_fd:	socket this server is listening to.
// buffer:		message to be sent.
// all:			true if including this server.
// self:		this server as a sockaddr_in struct.
void forward_to_servers(int listen_fd, char* buffer, bool all, sockaddr_in self) {
	for (int i = 0; i < SERVERS.size(); i++) {
		if (!all && i == SELF_INDEX - 1) {
			continue;
		}
		sendto(listen_fd, buffer, strlen(buffer), 0, (struct sockaddr*) &SERVERS[i], sizeof(SERVERS[i]));		
	}
}

// deliver messages to clients according to fifo ordering.
// listen_fd:	socket this server is listening to.
// sender:		index of the server that sent the message.
// msg_id:		sequence number of the message.
// room:		chat room where the message will be sent.
// message:		content of the message.
void handle_fifo_order(int listen_fd, int sender, int msg_id, int room, char* message) {
	// to zero indexing
	int group = room - 1;
	int node = sender - 1;
	// put message into holdback queue.
	HOLD_BACK_QUEUE_FIFO[group][node][msg_id] = string(message);

	int next = RECEIVED[group][node] + 1;
	// send all deliverable messages.
	while (HOLD_BACK_QUEUE_FIFO[group][node].find(next) != HOLD_BACK_QUEUE_FIFO[group][node].end()) {
		const char* m = HOLD_BACK_QUEUE_FIFO[group][node][next].c_str();
		char msg[MSG_LEN];
		strcpy(msg, m);
		forward_to_clients(listen_fd, msg, room);
		HOLD_BACK_QUEUE_FIFO[group][node].erase(next);
		RECEIVED[group][node]++;
		next = RECEIVED[group][node] + 1;
	}
}

// deliver messages to clients following causal order.
// listen_fd:	socket this server is listening to.
// sender:		index of the server that sent the message.
// clock:		vector clock string attached to the message.
// room:		chat room where the message will be sent.
// message:		content of the message.
void handle_causal_order(int listen_fd, int sender, char* clock, int room, char* message) {
	int group = room - 1;
	int node = sender - 1;
	// put message into holdback queue.
	Message m(sender, clock);
	HOLD_BACK_QUEUE_CAUSAL[group].push_back(m);

	while (true) {
		bool progress = false;
		for (int i = 0; i < HOLD_BACK_QUEUE_CAUSAL[group].size(); i++) {
			Message msg = HOLD_BACK_QUEUE_CAUSAL[group][i];

			if (msg.clock[node] == CLOCK[node] + 1 && have_seen_all(msg, node)) {
				forward_to_clients(listen_fd, message, room);
				CLOCK[node]++;
				HOLD_BACK_QUEUE_CAUSAL[group].erase(HOLD_BACK_QUEUE_CAUSAL[group].begin() + i);
				progress = true;
			}
		}

		if (!progress) {
			break;
		}
	}
}

// handling a server's message if the ordering is total order.
// listen_fd:	socket this server is listening to.
// sender:		index of the server that sent the message.
// type:		new message, proposal, or agreement.
// proposed_by:	index of the server that made the proposal.
// source:		source server where the original message was from.
// msg_id:		sequance number of the message.
// room:		chat room where the message will be sent.
// message:		content of the message.
// proposals:	all of the proposals for all messages this server has received so far.
void handle_total_order(int listen_fd, int sender, int type, int proposed_by, int msg_id, int room, char* message, unordered_map< string, vector< Message > >& proposals) {
	// convert to 0 indexing.
	int group = room - 1;
	int node = sender - 1;
	int self = SELF_INDEX - 1;
	char msg[MSG_LEN];
	string s(message);

	if (type == NEW_MESSAGE) {
		// propose a number if the message is a new message.
		PROPOSED[group] = max(PROPOSED[group], AGREED[group]) + 1;
		Message m(PROPOSED[group], 0);
		HOLD_BACK_QUEUE_TOTAL[group][m] = s;
		sprintf(msg, "%s,%d,%d,%d,%d,%s", "n/a", PROPOSAL, SELF_INDEX, PROPOSED[group], room, message);
		sendto(listen_fd, msg, strlen(msg), 0, (struct sockaddr*) &SERVERS[node], sizeof(SERVERS[node]));

	} else if (type == PROPOSAL) {
		// keep track of proposals if the message is sent by this server.
		if (proposals.find(s) == proposals.end()) {
			vector< Message > v;
			proposals[s] = v;
		}
		Message m(msg_id, proposed_by);
		proposals[s].push_back(m);

		// if all proposals are received, pick the maximum.
		if (proposals[s].size() == SERVERS.size()) {
			int max = 0;
			int max_by = 0;
			for (int i = 0; i < proposals[s].size(); i++) {
				// tie breaker
				if (proposals[s][i].id > max || (proposals[s][i].id == max && proposals[s][i].sender < max_by)) {
					max = proposals[s][i].id;
					max_by = proposals[s][i].sender;
				}
			}
			sprintf(msg, "%s,%d,%d,%d,%d,%s", "n/a", AGREEMENT, max_by, max, room, message);
			forward_to_servers(listen_fd, msg, true, SERVERS[self]);
			proposals.erase(s);
		}

	} else if (type == AGREEMENT) {
		// update the message in the holdback queue and agreed number.
		Message m;
		m.id = msg_id;
		m.sender = proposed_by;
		m.deliverable = true;
		for (auto iterator : HOLD_BACK_QUEUE_TOTAL[group]) {
			if (iterator.second.compare(s) == 0) {
				HOLD_BACK_QUEUE_TOTAL[group].erase(iterator.first);
				HOLD_BACK_QUEUE_TOTAL[group][m] = s;
			}
		}
		AGREED[group] = max(AGREED[group], msg_id);

		// deliver all deliverable messages.
		while (HOLD_BACK_QUEUE_TOTAL[group].begin()->first.deliverable) {
			strcpy(msg, HOLD_BACK_QUEUE_TOTAL[group].begin()->second.c_str());
			forward_to_clients(listen_fd, msg, room);
			HOLD_BACK_QUEUE_TOTAL[group].erase(HOLD_BACK_QUEUE_TOTAL[group].begin());
		}
	}
}

// check if this server has seen everything the sender has seen.
// msg:		Message wrapper including properties of a message.
// sender:	sender of the message.
// return:	true if this server has seen everything the sender has seen, false otherwise.
bool have_seen_all(Message& msg, int sender) {
	for (int i = 0; i < CLOCK.size(); i++) {
		if (i == sender) {
			continue;
		}

		if (msg.clock[i] > CLOCK[i]) {
			return false;
		}
	}

	return true;
}

// add time and server index to a debug message.
string debug_prefix() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* ptm;
	ptm = localtime(&tv.tv_sec);

	char time[16];
	strftime(time, sizeof(time), "%H:%M:%S", ptm);
	char micro[16];
	sprintf(micro, ".%06ld", tv.tv_usec);
	strcat(time, micro);

	char s[3];
	sprintf(s, "S%02d", SELF_INDEX);
	string server_str = string(s);

	return string(time) + " " + server_str;
}
