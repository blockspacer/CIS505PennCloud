#ifndef __test_h__
#define __test_h__

#include <resolv.h>
 #include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>


// #include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


// For DNS
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/nameser.h>
#define panic(a...) do { fprintf(stderr, a); fprintf(stderr, "\n"); exit(1); } while (0) 

// For each connection we keep a) its file descriptor, and b) a buffer that contains
// any data we have read from the connection but not yet processed. This is necessary
// because sometimes the server might send more bytes than we immediately expect.

struct connection {
  int fd;
  char *buf;
  int bytesInBuffer;
  int bufferSizeBytes;
};

// void log(const char *prefix, const char *data, int len, const char *suffix);
void writeString(struct connection *conn, const char *data);
// void expectNoMoreData(struct connection *conn);
int connectToPort(struct connection *conn, const char *DomainName, bool &UPENN_flag, bool &QQ_flag, bool &Outlook_flag, bool & Gmail_flag, bool &Hotmail_flag);
bool expectToRead(struct connection *conn, const char *data);
void expectRemoteClose(struct connection *conn);
void initializeBuffers(struct connection *conn, int bufferSizeBytes);
void closeConnection(struct connection *conn);
void freeBuffers(struct connection *conn);
void parse_record (unsigned char *buffer, size_t r, const char *section, ns_sect s, int idx, ns_msg *m, char * returnstr);

// lyz added

#define BufferSizeBytes (5000)
#define MAX_LEN_CMD (1024)

#endif /* defined(__test_h__) */
