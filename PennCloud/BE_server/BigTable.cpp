/*
 * BigTable.cpp
 *
 *  Created on: Nov 9, 2017
 *      Author: cis505
 */



#include "BigTable.h"

using namespace std;

BigTable::BigTable() {
	cout << "This is default constructor" << endl;
}

BigTable::BigTable(string server_id) {
	boost::filesystem::path dir(server_id);
	if (boost::filesystem::exists(dir)) {
		cout << "Directory" << server << "already exists" << endl;
	} else {
		boost::filesystem::create_directory(dir);
		cout << "Server Directory" << server << " is created" << endl;
	}
	mem_pos = 0;
	disk_file_index = 0;
	server = server_id;
}

BigTable::BigTable(string server_id, string memspace_checkpoint, string bigtable_checkpoint, string deleted_files_checkpoint) {
	server = server_id;
	load_memspace(server_id + "/" + memspace_checkpoint);
	load_bigtable(server_id + "/" + bigtable_checkpoint);
	load_deleted_files(server_id + "/" + deleted_files_checkpoint);
}



bool BigTable::exists(string username, string filename) {
	cerr << "Checking if file exists (" << username << ", " << filename << ")" << endl;
	if (bigtable.find(username) != bigtable.end() && bigtable.at(username).find(filename) != bigtable.at(username).end()) {
		return true;
	} else {
		return false;
	}
}

bool BigTable::put_helper(string username, string filename, string filetype, int filesize, const char content[]) {
	// check if file already exists
	if (exists(username, filename)) {
		cout << username << "/" << filename << " already exists\n";
		return false;
	} else {
		cerr << "Adding new file to big table" << endl;
		if (bigtable.find(username) == bigtable.end()) {
			cerr << "Adding row in big table for user: " << username << endl;
			unordered_map<string, Metadata> fileMap;
			bigtable.emplace(piecewise_construct, forward_as_tuple(username), forward_as_tuple());
		}
		if (bigtable.find(username) != bigtable.end() && bigtable[username].find(filename) != bigtable[username].end()) {
			cerr << "Erasing the file '" << filename << "' to overwrite it" << endl;
			bigtable[username].erase(filename);
		}

		cerr << "Putting file '" << filename << "' into row for user '" << username << "'" << endl;
		Metadata md(username, filename, filetype, to_string(disk_file_index), mem_pos, filesize, false);
		cerr << "Bigtable user count: " << bigtable.size() << endl;
		cerr << username << " file count: " << bigtable.at(username).size() << endl;
		bigtable.at(username).emplace(filename, md);
		cerr << "Completed inserting file into big table" << endl;
	}
	// if there are enough memory space, write content to memory
	if (mem_pos + filesize < MAX_BUFFER_SIZE) {
		cerr << "BigTable->put(): Writing content to memory" << endl;
		for (int i = 0; i < filesize; i++) {
			memspace[mem_pos++] = content[i];
		}
		string mem_filename = username + "/" + filename;
		mem_files.push_back(mem_filename);
		return true;
	} else {
		cerr << "BigTable->put(): Writing content to disk" << endl;
		// fluch current data in memory into disk first
		string disk_filename = server + "/" + to_string(disk_file_index);
		ofstream file(disk_filename);
		if (file.is_open()) {
			file.write((char*)&memspace[0], mem_pos);
			file.write((char*)&content[0], filesize);
		}
		file.close();
		// mark all the files in memory as flushed before clear the memory
		mem_files.push_back(username + "/" + filename);
		for (int i = 0; i < mem_files.size(); i++) {
			size_t slash = mem_files[i].find("/");
			string usern = mem_files[i].substr(0, slash);
			string filen = mem_files[i].substr(slash + 1);
			bigtable.at(usern).at(filen).is_flushed = true;
		}
		// assign a mutex to new created disk file
		disk_files.emplace(to_string(disk_file_index), mem_files);
		file_mutex.emplace(piecewise_construct, forward_as_tuple(to_string(disk_file_index)), forward_as_tuple());
		// clear memory
		mem_files.clear();
		memset(memspace, 0, mem_pos);
		mem_pos = 0;
		disk_file_index++;
		return true;
	}
}

int BigTable::get_filesize(string username, string filename) {
	if (exists(username, filename)) {
		return bigtable.at(username).at(filename).filesize;
	} else {
		return -1;
	}
}

unordered_map<string, Metadata> BigTable::get_user_files(string username) {
	cerr << "Getting files for user: " << username << endl;
	if (bigtable.find(username) != bigtable.end()) {
		return bigtable.at(username);
	}
	else {
		unordered_map<string, Metadata> emptyMap;
		return emptyMap;
	}
}

vector<string> BigTable::get_users() {
	vector<string> users;

	for (auto userRow = bigtable.begin(); userRow != bigtable.end(); userRow++) {
		cout << "User: " << userRow->first << endl;

		users.push_back(userRow->first);
	}

	return users;
}

string BigTable::get(string username, string filename) {
	int filesize = get_filesize(username, filename);
	string content = "";

	if (filesize < 0) {
		return content;
	} else {
		char* res = new char[filesize + 1];
		Metadata md = bigtable.at(username).at(filename);
		int size = filesize + 1;

		if (!md.is_flushed) {
			if (size > md.filesize) {
				size = md.filesize;
			}
			for (int i = 0; i < size; i++) {
				res[i] = memspace[md.start + i];
			}
            res[size] = 0;
		} else {
			// read from disk
			string disk_filename = server + "/" + md.disk_filename;
			// require lock first
			boost::shared_lock<boost::shared_mutex> lock(file_mutex.at(md.disk_filename));
			// start reading
			int fd;
			fd = open(disk_filename.c_str(), O_RDONLY);
			if (fd == -1) {
				cout << "Failed to read disk file" << disk_filename << endl;
				return content;
			}
			struct stat sb;
			if (fstat(fd, &sb) == -1) {
				cout << "Failed to get file statics" << endl;
				return content;
			}
			off_t offset, pa_offset;
			offset = md.start;
			pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
			if (offset >= sb.st_size) {
				fprintf(stderr, "offset is past end of file\n");
				return content;
			}

			if (size > md.filesize) {
				size = md.filesize;
			}
			char *addr;
			addr = (char*)mmap(NULL, size + offset - pa_offset, PROT_READ, MAP_PRIVATE, fd, pa_offset);
			if (addr == MAP_FAILED) {
				cout << "s failed\n";
				return content;
			}

			for (int i = 0; i < size; i++) {
				res[i] = *(addr + offset - pa_offset + i);
			}
			res[size] = 0;
			munmap(addr, size + offset - pa_offset);
			close(fd);
		}
		string s_content(res,size);
//		content = res;
		delete(res);
		return s_content;
	}
}

bool BigTable::dele(string username, string filename) {
	// lasy delete
	if (!exists(username, filename)) {
		return false;
	}
	Metadata md = bigtable.at(username).at(filename);
	string disk_filename = md.disk_filename;
	if (deleted_files.find(disk_filename) == deleted_files.end()) {
		deleted_files.emplace(piecewise_construct, forward_as_tuple(disk_filename), forward_as_tuple());
	}
	deleted_files.at(disk_filename).push_back(md);
	bigtable.at(username).erase(filename);
	return true;
}

bool BigTable::cput(string username, string filename, const char old_content[], int old_size, const char new_content[], int new_size, string new_type) {
	int size = get_filesize(username, filename);
	if (size != old_size) return false;

	string tmp = get(username, filename);
	for (int i = 0; i < size; i++) {
		if (tmp[i] != old_content[i]) {
			return false;
		}
	}
	dele(username, filename);
	return put_helper(username, filename, new_type, new_size, new_content);
}

bool BigTable::put(string username, string filename, string filetype, int filesize, const char content[]) {
	string old_content = get(username, filename);
	if (old_content.compare("") == 0) {
		return put_helper(username, filename, filetype, filesize, content);
	} else {
		return cput(username, filename, old_content.c_str(), old_content.size(), content, filesize, filetype);
	}
}

bool BigTable::dele_disk() {
	cerr << "starting delete disk...................." << endl;
    cerr << "delete_files.size :" << deleted_files.size() << endl;
	// delete all the files stored in deleted_files
	// disk_file_index: vector<Metadata>
    cerr << "disk_file_index:" << disk_file_index << endl;

    for (auto &entry : deleted_files) {
    	for (auto &second : entry.second) {
    		cerr << "deleted_files[" << entry.first << "]: filename: " << second.filename << endl;
    	}
    }

	for (auto iter = deleted_files.begin(); iter != deleted_files.end();) {
		string disk_fn = iter->first;
        cerr << "delting disk: " << disk_fn << endl;
		cerr << "files to be delete is:" << iter->second.size() << endl;
		int index = atoi(disk_fn.c_str());
		// if file are stored in memory, do nothing
		if (index == disk_file_index) continue;
		// if file is stored in disk
		boost::upgrade_lock<boost::shared_mutex> lock(file_mutex.at(disk_fn));
		boost::upgrade_to_unique_lock<boost::shared_mutex> uiqueLock(lock);
		dele_file(iter);
		iter->second.erase(iter->second.begin(), iter->second.end());
        if (iter->second.size() == 0) {
            iter = deleted_files.erase(iter);
        }
	}
	cerr << "ending delete disk...................." << endl;
	return true;
}

class Comparison {
public:
	bool operator() (pair<int, string> lhs, pair<int, string> rhs) {
		return lhs.first > rhs.first;
	}
};

bool BigTable::dele_file(auto iter) {
    cerr << ".............start_dele_file................." << endl;
	// sort files according to start position in the disk file
	priority_queue<pair<int, string>, vector<pair<int, string> >, Comparison> pq;
	vector<Metadata> files = iter->second;
    cerr << "b1" << endl;
	for (int i = 0; i < files.size(); i++) {
		int start = files[i].start;
		string val = files[i].username + "/" + files[i].filename;
        cerr << "deleting file:" << val << " starting from pos:" << start << endl;
		pq.push(make_pair(start, val));
	}
	cerr << "b2" << endl;
	string disk_fn = server + "/" + iter->first;
	ifstream file(disk_fn, ifstream::binary);
	cerr << "b3" << endl;
	file.seekg(0, file.end);
	int length = file.tellg();
	file.seekg(0, file.beg);
	char* buffer = new char[length + 1];
	file.read(buffer, length);
	file.close();
	cerr << "b4" << endl;
	ofstream ofile(disk_fn);
	int ptr = 0;
	if (ofile.is_open()) {
		for (int i = 0; i < disk_files.at(iter->first).size(); i++) {
			string s = disk_files.at(iter->first)[i];
			if (s == pq.top().second) {
				pair<int, string>  tmp = pq.top();
				cout << to_string(tmp.first) << tmp.second << endl;
				pq.pop();
                disk_files.at(iter->first).erase(disk_files.at(iter->first).begin() + i);
                i--;
			} else {
				size_t p = s.find("/");
				string usern = s.substr(0, p);
				string filen = s.substr(p + 1);
				int start = bigtable.at(usern).at(filen).start;
				int length = bigtable.at(usern).at(filen).filesize;
				ofile.write(&buffer[start], length);
				bigtable.at(usern).at(filen).start = ptr;
				ptr += length;
			}
		}
	}
	ofile.close();
	// if all the data are deleted, we can clear this disk file
	if (ptr == 0) {
		boost::filesystem::path dir(disk_fn);
		boost::filesystem::remove_all(disk_fn);
	}
	delete[] buffer;
    cerr << ".............end_dele_file................." << endl;
	return true;
}

void BigTable::print() {
	for (auto iter = bigtable.begin(); iter != bigtable.end(); iter++) {
		cout << "User: " << iter->first << endl;
		for (auto it  = iter->second.begin(); it != iter->second.end(); it++) {
			string val = get(iter->first, it->first);
			cout << "key: " << it->first << "   val: " << val << endl;
		}
	}

}


bool BigTable::write_memspace_to_disk() {
	cerr << "start write_memspace_to_disk...................." << endl;
	string path = server + "/" + MEMFILE;
	memspace_mutex.lock();
	ofstream ofs(path);
	if (ofs.is_open()) {
		ofs.write(&memspace[0], mem_pos);
		ofs.close();
		memspace_mutex.unlock();
		return true;
	} else {
		memspace_mutex.unlock();
		cerr << "Can not open file:" << path << endl;
		return false;
	}
	cerr << "end write_memspace_to_disk...................." << endl;
}


bool BigTable::write_bigtable_to_disk() {
	cerr << "start write_bigtable_to_disk...................." << endl;
	string path = server + "/" + BIGTABLEFILE;
	bigtable_mutex.lock();
	ofstream ofs(path);

	if (ofs.is_open()) {
		for (auto iter1 = bigtable.begin(); iter1 != bigtable.end(); iter1++) {
			for (auto iter2 = iter1->second.begin(); iter2 != iter1->second.end(); iter2++) {
				ofs << iter2->second.username << ";";
				ofs << iter2->second.filename << ";";
				ofs << iter2->second.filetype << ";";
				ofs << iter2->second.disk_filename << ";";
				ofs << to_string(iter2->second.start) << ";";
				ofs << to_string(iter2->second.filesize) << ";";
				ofs << to_string(iter2->second.is_flushed) << endl;
			}
		}
		ofs.close();
		bigtable_mutex.unlock();
		return true;

	} else {
		bigtable_mutex.unlock();
		cerr << "Can not open file:" << path << endl;
		return false;
	}

}


bool BigTable::write_deleted_files_to_disk() {
	cerr << "start write_delted_files_to_disk...................." << endl;
	string path = server + "/" + DELETEFILE;
	deleted_files_mutex.lock();
	ofstream ofs(path);

	if (ofs.is_open()) {
		for (auto iter = deleted_files.begin(); iter != deleted_files.end(); iter++) {
			string disk_filename = iter->first;

			for (int i = 0; i < iter->second.size(); i++) {
//				ofs << disk_filename << ";";
				ofs << iter->second[i].username << ";";
				ofs << iter->second[i].filename << ";";
				ofs << iter->second[i].filetype << ";";
				ofs << iter->second[i].disk_filename << ";";
				ofs << to_string(iter->second[i].start) << ";";
				ofs << to_string(iter->second[i].filesize) << ";";
				ofs << to_string(iter->second[i].is_flushed) << endl;
			}
		}
		ofs.close();
		deleted_files_mutex.unlock();
		return true;
	} else {
		deleted_files_mutex.unlock();
		cerr << "Can not open file:" << path << endl;
		return false;
	}
}


bool BigTable::load_memspace(string memspace_fn) {
	memspace_mutex.lock();
	mem_pos = 0;
	int fd;
	fd = open(memspace_fn.c_str(), O_RDONLY);
	if (fd == -1) {
		cerr << "Can not open file:" << memspace_fn << endl;
		memspace_mutex.unlock();
		return false;
	}

	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		cerr << "Can not get fle statics" << endl;
		memspace_mutex.unlock();
		return false;
	}

	if (sb.st_size == 0) {
		cerr << "Memspace is empty" << endl;
		memspace_mutex.unlock();
		return true;
	}
	off_t offset = 0;
	off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);

	char *addr;
	addr = (char*) mmap(NULL, sb.st_size + offset - pa_offset, PROT_READ, MAP_PRIVATE, fd, pa_offset);
	if (addr == MAP_FAILED) {
		cerr << "mmap failed\n";
		memspace_mutex.unlock();
		return false;
	}

	for (int i = 0; i < sb.st_size; i++) {
		memspace[i] = *(addr + offset - pa_offset + i);
		mem_pos++;
	}

	munmap(addr, sb.st_size + offset - pa_offset);
	close(fd);
	memspace_mutex.unlock();
	return true;
}


bool BigTable::load_bigtable(string bigtable_fn) {
	bigtable_mutex.lock();
	ifstream ifs(bigtable_fn);
	disk_file_index = 0;

	if (!ifs.is_open()) {
		cerr << "Can not open file:" << bigtable_fn << endl;
		bigtable_mutex.unlock();
		return false;
	}

	bool has_in_memory_file = false;

	string line;
	while (getline(ifs, line)) {
		istringstream ss(line);
		string token;
		vector<string> fields;
		while (getline(ss, token, ';')) {
			fields.push_back(token);
		}
		if (fields.size() != 7) continue;
		string username = fields[0];
		string filename = fields[1];
		string filetype = fields[2];
		string disk_filename = fields[3];
		int start = atoi(fields[4].c_str());
		int filesize = atoi(fields[5].c_str());
		bool is_flushed = atoi(fields[6].c_str());

		disk_file_index = max(disk_file_index, atoi(disk_filename.c_str()));
		Metadata md(username, filename, filetype, disk_filename, start, filesize, is_flushed);
		if (bigtable.find(username) == bigtable.end()) {
			bigtable.emplace(piecewise_construct, forward_as_tuple(username), forward_as_tuple());
		}
		bigtable.at(username).emplace(filename, md);

		if (is_flushed) {
			if (disk_files.find(disk_filename) == disk_files.end()) {
				disk_files.emplace(piecewise_construct, forward_as_tuple(disk_filename), forward_as_tuple());
			}
			disk_files.at(disk_filename).push_back(username + "/" + filename);

			if (file_mutex.find(disk_filename) == file_mutex.end()) {
				file_mutex.emplace(piecewise_construct, forward_as_tuple(disk_filename), forward_as_tuple());
			}
		} else {
			mem_files.push_back(username + "/" + filename);
			has_in_memory_file = true;
		}
	}

	ifs.close();
	if (!has_in_memory_file) {
		disk_file_index++;
	}
	bigtable_mutex.unlock();
}


bool BigTable::load_deleted_files(string deleted_files_fn) {

	deleted_files_mutex.lock();
	ifstream ifs(deleted_files_fn);
	if (!ifs.is_open()) {
		cerr << "Can not open file:" << deleted_files_fn << endl;
		deleted_files_mutex.unlock();
		return false;
	}


	string line;
	while (getline(ifs, line)) {
		istringstream ss(line);
		string token;
		vector<string> fields;
		while (getline(ss, token, ';')) {
			fields.push_back(token);
		}
		if (fields.size() != 7) continue;

		string username = fields[0];
		string filename = fields[1];
		string filetype = fields[2];
		string disk_filename = fields[3];
		int start = atoi(fields[4].c_str());
		int filesize = atoi(fields[5].c_str());
		bool is_flushed = atoi(fields[6].c_str());

		if (deleted_files.find(disk_filename) == deleted_files.end()) {
			deleted_files.emplace(piecewise_construct, forward_as_tuple(disk_filename), forward_as_tuple());
		}
		Metadata md(username, filename, filetype, disk_filename, start, filesize, is_flushed);
		deleted_files.at(disk_filename).push_back(md);
	}
	ifs.close();
	deleted_files_mutex.unlock();
}


bool BigTable::checkpoint() {
	cerr << "starting checkpoint...................." << endl;
	return write_memspace_to_disk() && write_bigtable_to_disk() && write_deleted_files_to_disk();
	cerr << "starting checkpoint...................." << endl;
}







