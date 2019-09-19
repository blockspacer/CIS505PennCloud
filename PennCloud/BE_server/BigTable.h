/*
 * BigTable.h
 *
 *  Created on: Nov 9, 2017
 *      Author: cis505
 */


#ifndef BIGTABLE_H_
#define BIGTABLE_H_

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <mutex>
#include <tuple>
#include <fstream>
#include <utility>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <queue>

#define MEMFILE "memspace"
#define BIGTABLEFILE "bigtable"
#define DELETEFILE "deleted_files"

using namespace std;

const int MAX_BUFFER_SIZE = 4;


class Metadata {
public:
	string username;
	string filename;
	string filetype;
	string disk_filename;
	int start;
	int filesize;
	bool is_flushed;

	Metadata(string username, string filename, string filetype, string disk_filename, int start, int size, bool is_flushed) {
		this->username = username;
		this->filename = filename;
		this->filetype = filetype;
		this->disk_filename = disk_filename;
		this->start = start;
		this->filesize = size;
		this->is_flushed = is_flushed;
	}
};

class BigTable {
	// current server ID
	string server;
	// in memory files
	char * memspace = new char[MAX_BUFFER_SIZE];
	// current position in memspace
	int mem_pos;
	// index of disk files
	int disk_file_index;
	// mapping from username to the files belong to this user
	unordered_map<string, unordered_map<string, Metadata>> bigtable;
	// memory files in format "username/filename"
	vector<string> mem_files;
	// mapping from disk_file_index to mem_files that are written into disk file
	unordered_map<string, vector<string> > disk_files;
	// mapping from disk_file_index to deleted files (time, metadata)
	unordered_map<string, vector<Metadata>> deleted_files;
	// mutex for each disk file
	unordered_map<string, boost::shared_mutex> file_mutex;

public:
	mutex log_mutex;
	mutex memspace_mutex;
	mutex bigtable_mutex;
	mutex deleted_files_mutex;

public:
    // rowkey = username
    // colkey = filename
    // value = content

    // constructor
	BigTable();
	BigTable(string server_id);
	BigTable(string server, string memspace_checkpoint, string bigtable_checkpoint, string deleted_files_checkpoint);

    // helper function
	bool exists(string username, string filename);
	bool put_helper(string username, string filename, string filetype, int filesize, const char content[]);
	int get_filesize(string username, string filename);
	unordered_map<string, Metadata> get_user_files(string username);
	vector<string> get_users();

	// four basic operation
	string get(string username, string filename);
	bool cput(string username, string filename, const char old_content[], int old_size, const char new_content[], int new_size, string new_type);
	bool dele(string username, string filename); // lazy deletion
	bool put(string username, string filename, string filetype, int filesize, const char content[]);

	// disk clearance function
	bool dele_disk();
	bool dele_file(auto iter);

    // debug function
	void print();


	// check point helper functions
	bool checkpoint();
	bool write_memspace_to_disk();
	bool write_bigtable_to_disk();
	bool write_deleted_files_to_disk();

	// recovery helper functions
	bool load_memspace(string memspace_fn);
	bool load_bigtable(string bigtable_fn);
	bool load_deleted_files(string deleted_files_fn);
};



#endif /* BIGTABLE_H_ */
