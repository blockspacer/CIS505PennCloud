#include "smtp_common.h"

using namespace std;
void writeString(struct connection *conn, const char *data)
{
    int len = strlen(data);
    printf("C: %s", data);
    int wptr = 0;
    while (wptr < len) {
        int w = write(conn->fd, &data[wptr], len-wptr);
        if (w<0) panic("Cannot write to conncetion (%s)", strerror(errno));
        if (w==0) panic("Connection closed unexpectedly");
        wptr += w;
    }
}

// HW2: test/common.cc
bool expectToRead(struct connection *conn, const char *data)
{
  // Keep reading until we see a LF

  int lfpos = -1;
  while (true) {
    for (int i=0; i<conn->bytesInBuffer; i++) {
      if (conn->buf[i] == '\n') {
        lfpos = i;
        break;
      }
    }

    if (lfpos >= 0)
      break;

    if (conn->bytesInBuffer >= conn->bufferSizeBytes)
      panic("Read %d bytes, but no CRLF found", conn->bufferSizeBytes);

    int bytesRead = read(conn->fd, &conn->buf[conn->bytesInBuffer], conn->bufferSizeBytes - conn->bytesInBuffer);
    if (bytesRead < 0)
      panic("Read failed (%s)", strerror(errno));
    if (bytesRead == 0)
      panic("Connection closed unexpectedly");

    conn->bytesInBuffer += bytesRead;
  }

  printf("S: %s", conn->buf);

  // Get rid of the LF (or, if it is preceded by a CR, of both the CR and the LF)

  bool crMissing = false;
  if ((lfpos==0) || (conn->buf[lfpos-1] != '\r')) {
    crMissing = true;
    conn->buf[lfpos] = 0;
  } else {
    conn->buf[lfpos-1] = 0;
  }

  // Check whether the server's actual response matches the expected response
  // Note: The expected response might end in a wildcard (*) in which case
  // the rest of the server's line is ignored.

  int argptr = 0, bufptr = 0;
  bool match = true;
  while (match && data[argptr]) {
    if (data[argptr] == '*') 
      break;
    if (data[argptr++] != conn->buf[bufptr++])
      match = false;
  }

  if (!data[argptr] && conn->buf[bufptr])
    match = false;

  // Annotate the output to indicate whether the response matched the expectation.

  if (match) {
    if (crMissing)
      printf(" [Terminated by LF, not by CRLF]\n");
    else 
      printf(" [OK]\n");
  } else {
    printf(" [Expected: '%s']\n", data);
  }

  // 'Eat' the line we just parsed. However, keep in mind that there might still be
  // more bytes in the buffer (e.g., another line, or a part of one), so we have to
  // copy the rest of the buffer up.

  for (int i=lfpos+1; i<conn->bytesInBuffer; i++)
    conn->buf[i-(lfpos+1)] = conn->buf[i];
  conn->bytesInBuffer -= (lfpos+1);

  return true;
}


int connectToPort(struct connection *conn, const char *DomainName, bool &UPENN_flag, bool &QQ_flag, bool &Outlook_flag, bool & Gmail_flag, bool &Hotmail_flag)
{
    
  // http://www.sourcexr.com/articles/2013/10/12/dns-records-query-with-res_query
    printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
    // First, we make the DNS query using res_query, the generic method that allows us to supply the type of record we are interested in:
    const size_t size = 1024;
    unsigned char buffer[size];
    int r = res_query (DomainName, C_IN, T_MX, buffer, size);
    if (r == -1) { fprintf(stderr,  "%d %s\n",errno, strerror (errno) ); return 1; }
    else {
        if (r == static_cast<int> (size)) { fprintf(stderr,"Buffer too small reply truncated\n"); return 1; }
    }
    // Then we check the reply for errors sent by the server:
    HEADER *hdr = reinterpret_cast<HEADER*> (buffer);
    // And we handle now the reply. First we read the header to get the number of each of records:
    int question = ntohs (hdr->qdcount);
    int answers = ntohs (hdr->ancount);
    int nameservers = ntohs (hdr->nscount);
    int addrrecords = ntohs (hdr->arcount);
    printf("Reply: question: %d, answers: %d, nameservers: %d, address records: %d\n" 
      ,question, answers, nameservers, addrrecords );
    // Then we go through each record in turn: first the question specifically, then the last three using the same parse function. The resolv library supplies several helpers to read the received buffer:
    ns_msg m;
    int k = ns_initparse (buffer, r, &m);
    if (-1 == k) { fprintf(stderr,"%d %s\n", errno, strerror (errno)); return 1; }
    char returnstr[100];
// We are now ready to read the reply: first the question record using macros to access the fields of the ns_rr data structure and then the remaining records.
    //  for (int i = 0; i < question; ++i) {
    //     ns_rr rr;
    //     int k = ns_parserr (&m, ns_s_qd, i, &rr);
    //     if (-1 == k) { fprintf(stderr,"%d %s\n", errno, strerror (errno)); return 1; }
    //     cout << "question " << ns_rr_name (rr) << " "
    //               << ns_rr_type (rr) << " " << ns_rr_class (rr) << "\n";
    // }
    for (int i = 0; i < answers; ++i) {
        parse_record (buffer, r, "answers", ns_s_an, i, &m,returnstr);
        printf("##########second time:%s\n",returnstr);
    }

    // for (int i = 0; i < nameservers; ++i) {
    //     parse_record (buffer, r, "nameservers", ns_s_ns, i, &m,returnstr);
    // }
    // if (0 == returnstr[0]) {fprintf(stderr,"no useful MX record is found!!\n"); return 1;}
    // for (int i = 0; i < addrrecords; ++i) {
    //     parse_record (buffer, r, "addrrecords", ns_s_ar, i, &m, returnstr);
    // }
    // printf("USE BELOW -------------------------------------\n");
    // memset(returnstr,0,sizeof(returnstr));
    // for (int i =0; i < addrrecords; ++i) {
    //     parse_record (buffer, r, "addrrecords", ns_s_ar, i, &m, returnstr);
    //     if (0 != returnstr[0])  break;
    // }
    // printf("USE ABOVE --------%s-----------------------------\n",returnstr);
    hostent *host_HuangYi = NULL;
    if (string::npos != string(DomainName).find("upenn.edu")){UPENN_flag = true;}
    // if (0 == strcmp(DomainName,"seas.upenn.edu") )
    // {host_HuangYi  = gethostbyname("telepathy.seas.upenn.edu"); }
    // else if (0 == strcmp(DomainName,"sas.upenn.edu") )
    // { host_HuangYi  = gethostbyname("gi1.sas.upenn.edu"); }
    if (0 == strcmp(DomainName,"qq.com"))
    {//host_HuangYi  = gethostbyname("mx3.qq.com"); 
  QQ_flag = true; }
    if (0 == strcmp(DomainName,"outlook.com"))
    {//host_HuangYi  = gethostbyname("outlook-com.olc.protection.outlook.com"); 
      Outlook_flag = true;}
    if (0 == strcmp(DomainName,"gmail.com")){Gmail_flag = true;}
    if (0 == strcmp(DomainName, "hotmail.com")) {Hotmail_flag = true;}
    // else if (0 == strcmp(DomainName, "hotmail.com"))
    // {host_HuangYi  = gethostbyname("hotmail-com.olc.protection.outlook.com"); }
  //-----------------
    // else{printf("###########not sas seas QQ outlook hotmail####################333");
      // char returnstr[100];
    //   printf("USE BELOW -------------------------------------\n");
    //   memset(returnstr,0,sizeof(returnstr));
    //   for (int i = 0; i < answers; ++i) {
    //     parse_record (buffer, r, "answers", ns_s_an, i, &m,returnstr);
    //     host_HuangYi = gethostbyname(returnstr);
    //     if (NULL != host_HuangYi)  {break;}
    //     }
    //   printf("USE ABOVE --------%s-----------------------------\n",returnstr);
    // }
    printf("USE BELOW -------------------------------------\n");
    memset(returnstr,0,sizeof(returnstr));
    for (int i = 0; i < answers; ++i) {
      parse_record (buffer, r, "answers", ns_s_an, i, &m,returnstr);
      host_HuangYi = gethostbyname(returnstr);
      if (NULL != host_HuangYi)  {break;}
    }
    printf("USE ABOVE --------%s-----------------------------\n",returnstr);
    if (NULL == host_HuangYi) {fprintf(stderr,"no useful MX record is found!!\n"); return 1;}
    conn->fd = socket(PF_INET, SOCK_STREAM, 0);
    if (conn->fd < 0) panic("Cannot open socket (%s)", strerror(errno));
      //http://www.cplusplus.com/forum/articles/9742/
    in_addr * address = (in_addr * )host_HuangYi->h_addr;
    string ip_address = inet_ntoa(* address);
    //ip_address = "104.47.0.33"; // outlook sometimes get 104.47.6.33 -- and then fail

    printf("###WE are using ip address %s\n",ip_address.c_str());
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET; servaddr.sin_port=htons(25);
    inet_pton(AF_INET, ip_address.c_str(), &(servaddr.sin_addr)); //158.130.68.71
    // http://developerweb.net/viewtopic.php?id=3196
    fcntl(conn->fd, F_SETFL, O_NONBLOCK);
    if (connect(conn->fd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
    { 
     if (errno == EINPROGRESS) { 
        fd_set fdset; FD_ZERO(&fdset); FD_SET(conn->fd, &fdset);
        struct timeval tv;  tv.tv_sec = 10; tv.tv_usec = 0;
        int so_error;
        if (select(conn->fd+1, NULL, &fdset, NULL, &tv) > 0) { 
            socklen_t len = sizeof(int); 
            getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, (void*)(&so_error), &len); 
            if (so_error) { 
              fprintf(stderr, "Error in connection() %d - %s\n", so_error, strerror(so_error)); 
              return 1;
           } 
        } 
        else { 
          fprintf(stderr, "Timeout or error() %d - %s\n", so_error, strerror(so_error)); 
          return 1; 
        } 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        return 1;
     } 
  } 
  // Set to blocking mode again... 
  long arg = fcntl(conn->fd, F_GETFL, NULL); 
  arg &= (~O_NONBLOCK); 
  fcntl(conn->fd, F_SETFL, arg); 
  // I hope that is all 
  return 0;  

    // fd_set fdset; FD_ZERO(&fdset); FD_SET(conn->fd, &fdset);
    // struct timeval tv;  tv.tv_sec = 10; tv.tv_usec = 0;
    //  if (select(conn -> fd + 1, NULL, &fdset, NULL, &tv) == 1)
    // {
    //     int so_error;
    //     socklen_t len = sizeof(so_error);

    //     getsockopt(conn -> fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    //     printf("?????????????????????????????????? second\n");
    //     if (so_error == 0) { printf("%s:%d is open\n", returnstr, 25);  return 1;}
    //     return 0;
    // } 
    // return 0;

}




// And finally the generic parse record function. It only parses a small subset of the available records (A, NS, and MX). It unpacks the domain name as they may be compressed in the record:

void parse_record (unsigned char *buffer, size_t r,
                   const char *section, ns_sect s,
                   int idx, ns_msg *m, char *returnstr) {
    ns_rr rr;
    int k = ns_parserr (m, s, idx, &rr);
    if (-1 == k) { fprintf(stderr,"%d %s\n", errno, strerror (errno)); return; }
    cout << section << " " << ns_rr_name (rr) << " "
              << ns_rr_type (rr) << " " << ns_rr_class (rr)
              << ns_rr_ttl (rr) << " ";
    const size_t size = NS_MAXDNAME;
    unsigned char name[size];
    int t = ns_rr_type (rr);
    const u_char *data = ns_rr_rdata (rr);
    if (t == T_MX) {
        int pref = ns_get16 (data);
        ns_name_unpack (buffer, buffer + r, data + sizeof (u_int16_t),
                        name, size);
        char name2[size];
        ns_name_ntop (name, name2, size);
        // cout << pref << " " << name2<< "ppppppppppppp";
        strcpy(returnstr, name2);
    }
    else if (t == T_A) {
        unsigned int addr = ns_get32 (data);
        struct in_addr in;
        in.s_addr = ntohl (addr);
        char *a = inet_ntoa (in);
        // cout << a << "aaaaaaaaaaaaaaaaaaaaaaaa";
        strcpy(returnstr,a);
    }

    else if (t == T_NS) {
        ns_name_unpack (buffer, buffer + r, data, name, size);
        char name2[size];
        ns_name_ntop (name, name2, size);
        // cout << name2 << "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn";
        strcpy(returnstr, name2);
    }else {cout << "unhandled record"; }
    cout << "\n";
}
