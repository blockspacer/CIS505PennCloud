//
// Created by cis505 on 11/21/17.
//

#ifndef T01_LOGGER_H
#define T01_LOGGER_H

#include <vector>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <sstream>

#include "../common/WriteService.grpc.pb.h"

using writeservice::WriteCommandRequest;

using namespace std;

class Logger {
    string log_file;
    pthread_mutex_t log_lock;

public:
    // constructor
    Logger(string file_path);

    // basic operation
    bool append(string entry);
    vector<WriteCommandRequest> get();
    bool clear();
    void print();

};

#endif //T01_LOGGER_H
