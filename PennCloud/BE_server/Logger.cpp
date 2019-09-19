//
// Created by cis505 on 11/21/17.
//
#include "Logger.h"
#include <fstream>

using namespace std;

/*
 * Constructor
 * @param file_path : the path of the log file
 */
Logger::Logger(string file_path) {
    log_file = file_path;
    log_lock = PTHREAD_MUTEX_INITIALIZER;
};

/*
 * append : write a new entry to the end of log file
 * @param entry : the new log record
 */
bool Logger::append(string entry) {
    pthread_mutex_lock(&log_lock);
    ofstream file(log_file, ios_base::app);
    if (!file.is_open()) {
        return false;
    }
    file << entry.size() << "," << entry;
    file.close();
    pthread_mutex_unlock(&log_lock);
    return true;
}

/*
 * get : read the whole lof file into a string object
 */
vector<WriteCommandRequest> Logger::get() {
	vector<WriteCommandRequest> writeCommands;
    pthread_mutex_lock(&log_lock);
    ifstream file(log_file, ios::binary);
    if (!file.is_open()) {
        return writeCommands;
    }
    file.seekg(0, file.end);
    size_t size = file.tellg();
    cerr << "filesize:" << size << endl;
    file.seekg(0, file.beg);

    string entrySize;
    while (getline(file, entrySize, ',')) {
    	cerr << "Log entry size: " << entrySize << endl;
    	int size = stoi(entrySize);

    	char logEntry[size];
    	file.read(logEntry, size);
    	string logStr(logEntry, size);
    	WriteCommandRequest request;
    	bool success = request.ParseFromString(logStr);
    	cerr << "Write command parse success: " << success << endl;
    	writeCommands.push_back(request);
    }

    file.close();
    pthread_mutex_unlock(&log_lock);
    return writeCommands;
}

/*
 * clear: clear the content of log file
 */
bool Logger::clear() {
    pthread_mutex_lock(&log_lock);
    ofstream ofs;
    ofs.open(log_file, ios::out | ios::trunc);
    ofs.close();
    pthread_mutex_unlock(&log_lock);
    return true;
}

void Logger::print() {
    vector<WriteCommandRequest> logEntries = get();
    cerr << "log is: " << endl;
    for (WriteCommandRequest request : logEntries) {
    	cerr << request.SerializeAsString() << endl;
    }
}

