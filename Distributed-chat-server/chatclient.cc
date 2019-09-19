#include <arpa/inet.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

using namespace std;

const int BUFFER_LEN = 1024;

// main function. Send messages to the address specified. Read from console input and socket.
int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "*** Author: Han Zhu (zhuhan)\n");
		exit(1);
	}

	// open socket
	int listen_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (listen_fd < 0) {
		cerr << "Cannot open socket" << endl;
		exit(2);
	}

	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;

	// parse command line argument and construct destination address.
	char* token = strtok(argv[1], ":");
	inet_pton(AF_INET, token, &dest.sin_addr);

	token = strtok(NULL, ":");
	if (token == NULL) {
		cerr << "Please enter valid IP and port" << endl;
		exit(3);
	}
	dest.sin_port = htons(atoi(token));

	struct sockaddr_in src;
	socklen_t src_len = sizeof(src);

	// main loop
	while (true) {
		// select from console input and socket.
		fd_set readset;
		int readable = 0;

		FD_ZERO(&readset);
		FD_SET(STDIN_FILENO, &readset);
		FD_SET(listen_fd, &readset);
		readable = select(listen_fd + 1, &readset, NULL, NULL, NULL);

		char buffer[BUFFER_LEN];
		int len = 0;
		
		if (FD_ISSET(STDIN_FILENO, &readset)) {
			len = read(STDIN_FILENO, buffer, BUFFER_LEN);
			buffer[len - 1] = 0;
			sendto(listen_fd, buffer, BUFFER_LEN, 0, (struct sockaddr*) &dest, sizeof(dest));

			char* token = strtok(buffer, " ");
			// return if user types "/quit".
			if (strcasecmp(token, "/quit") == 0) {
				return 0;
			}
		} else {
			len = recvfrom(listen_fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &src, &src_len);
			buffer[len] = 0;
			cout << buffer << endl;
		}
		
		memset(buffer, 0, BUFFER_LEN);
	}

	close(listen_fd);
	return 0;
}