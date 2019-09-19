/*
 * StorageRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "StorageRepositoryImpl.h"

using namespace std;


/* For a given user, get the list of file names under a specific folder
 * This method is used to display files summary (organized in directory structure) on webpage
 */
Status StorageRepositoryImpl::GetFileList(ServerContext* context, const GetFileRequest* request, ServerWriter<FileDTO>* writer){
	cerr<<"Received a GetFileList request for user "<<request->username()<<" under "<<request->filename()<<endl;
	string username = request->username();
	string identifier = request->filename();
	std::vector<string> unmatched;
	std::vector<filemeta> v = parseFileList(username, identifier, unmatched, bigTablePtr);

	for (auto it = v.begin(); it != v.end(); it++){
		struct filemeta m = *it;
		FileDTO f;
		f.set_owner(username);
		// f.set_filename(m.name);
		if (m.type == 1){
			size_t p = m.name.find(".");
			if (p == std::string::npos){
				f.set_type("unknown type file");
			} else {
				f.set_type(m.name.substr(p+1));
			}
			f.set_uid(m.fuid);
			f.set_time(m.time);
			f.set_size(m.size);
			f.set_filename(m.name);
		} else {
			f.set_type("folder");
			f.set_filename(m.name.substr(0,m.name.length()-1));
			if(f.filename().length() == 0) continue;
		}
		writer->Write(f);
		cerr<<"Found the in the given folder: "<<f.filename();
		cerr<<"Returning the following info: "<<f.SerializeAsString()<<endl;
	}

	return Status::OK;
}

/* Get file data by fuid */
Status StorageRepositoryImpl::GetFile(ServerContext* context, const GetFileRequest* request, FileDTO* file) {

	cerr << "Received a request to get a file" << endl;
	string username = request->username();
	string identifier = request->filename();  // fuid

	// if the file not exists, set uid to be empty and return
	if (!bigTablePtr->exists(username, identifier)){
		cerr<<"requested file not exist"<<endl;
		file->set_uid("");
		file->set_type("");
		file->set_data("");
		file->set_owner("");
		file->set_filename("");
		file->set_size(0);
	} else {
		cerr<<"requested file exists"<<endl;
		file->set_uid(identifier);
		string content = bigTablePtr->get(username, identifier);
		size_t find = content.find("\n");
		find = content.find("\n", find+1);
		file->set_data(content.substr(find+1));
		file->set_size(file->data().length());
		file->set_filename("");
		file->set_owner(username);
		file->set_type("");
	}

	return Status::OK;
}

/* Download all files uoder a given folder, not including subfolders */
Status StorageRepositoryImpl::GetFilesByFolder(ServerContext* context, const GetFileRequest* request,
		ServerWriter<FileDTO>* writer) {

	cerr << "Received a request to GetFilesByFolder"<<endl;
	string username = request->username();
	string folder = request->filename();

	std::vector<string> unmatched;
	std::vector<filemeta> v = parseFileList(username, folder, unmatched, bigTablePtr);
	for (auto it = v.begin(); it != v.end(); it++){
		struct filemeta m = *it;
		FileDTO f;
		if (m.type == 1){
			cerr<<"find a matched file: "<<"name="<<m.name<<endl<<"uid="<<m.fuid<<endl;
			f.set_filename(m.name);
			f.set_owner(username);
			f.set_uid(m.fuid);
			string content = bigTablePtr->get(username, m.fuid);
			size_t find = content.find("\n");
			find = content.find("\n", find+1);
			f.set_data(content.substr(find+1));
			f.set_size(f.data().length());
			writer->Write(f);
		}
	}

	return Status::OK;
}

/**
 * Add new file or replace old file (if filename is the same)
 */
Status StorageRepositoryImpl::AddFile(ServerContext* context, const FileDTO* request, AddFileResponse* response) {

	cerr<<"Received a request to add file"<<endl;
	string user = request->owner();
	// filename format: /folder(s)/filename
	string filename = request->filename();
	cerr<<"user="<<user<<" ; "<<"filename="<<filename<<endl;


	std::vector<string> unmatched;
	std::vector<filemeta> v = parseFileList(user, filename, unmatched, bigTablePtr);
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// delete old file data, not allow duplicate file names here
	if (v.size() > 0) {
		struct filemeta m = (filemeta)v.at(0);
		bigTablePtr->dele(user, m.fuid);
	}
	string entry = filename + "\t" + request->uid() + "\n";
	largestr.append(entry);
	cerr<<"add new entry to filelist: ("<<filename<<"/r"<<request->uid()<<"/n"<<"), count="<<entry.length()<<endl;
	char buffer[50];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%ld", request->size());
	string filecontent = string(buffer) + "\n" + request->time() + "\n" + request->data();
	bigTablePtr->put(user, request->uid(), request->type(), filecontent.length(), filecontent.c_str());
	cerr<<"put ("<<user<<","<<request->uid()<<") file in bigtable"<<endl;

	// update filelist in table
	bigTablePtr->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	// cerr << "-----------------begin the table ---------------------------" << endl;
	// bigTablePtr->print();
	// cerr << "------------------end the table ---------------------------" << endl;

	response->set_success(true);
	response->set_message("Successfully stored file " + request->filename());

	return Status::OK;
}


/**
 * Add a new folder(empty) in the current directory (root directory is '/').
 * Not allow 2 folders with the same name.
 * Treat empty folder as a special FileDTO with empty uid, and type "folder".
 * If the folder name has not been used, return success reponse.
 * Otherwise return failure response with error message.
 */
Status StorageRepositoryImpl::AddFolder(ServerContext* context, const FileDTO* request, AddFileResponse* response) {

	cerr<<"Received a request to add folder"<<endl;
	string user = request->owner();
	// format: /folder(s)/foldername
	string foldername = request->filename();

	std::vector<string> unmatched;
	std::vector<filemeta> v = parseFileList(user, foldername, unmatched, bigTablePtr);
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// do nothing if folder name is duplicate
	if (v.size() > 0) {
		response->set_success(false);
		response->set_message("Create folder failed, folder " + foldername + "already exists in current directory.");
		return Status::OK;
	}

	string entry = foldername + "\n";
	largestr.append(entry);
	cerr<<"add new entry to filelist: ("<<foldername<<"/n"<<"), count="<<entry.length()<<endl;

	// update filelist in table
	bigTablePtr->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	cerr << "--------------------begin the table ---------------------------" << endl;
	bigTablePtr->print();
	cerr << "--------------------end the table ---------------------------" << endl;

	response->set_success(true);
	response->set_message("Successfully created folder " + request->filename());

	return Status::OK;
}


 // TODO: deal with large file add/delete/getFileList

/** Find all files begin with '/filename', store their uids in a priority queue ordered by chunk sequence.
 *  Regular sized file (<2M) will have one match, large files will have multiple entries matched and each is a chunk file.
 *  The matched entries are chunk files belonging to a single large file (>2M).
 *  Unmatched entries are stored in umatched vector. --> not used
 */
vector<filemeta> handleLargeFileHelper(string user, string folder, vector<string>& unmatched, BigTable* bigTablePtr) {

	std::vector<filemeta> v;
	if (!bigTablePtr->exists(user, "filelist")) {
		return v;
	}
	string rawdata = bigTablePtr->get(user, "filelist");
	
	size_t len = folder.length();
	size_t start = 0;
	size_t pos = rawdata.find("\n");
	string line = "";
	while(pos != std::string::npos){
		line = rawdata.substr(start, pos-start);
		cerr<<"read line: "<<line<<endl;
		struct filemeta item;
		size_t found = line.find("\t");
		// lowest level is empty folder
		if (found == std::string::npos){

			if (line.compare(0, len, folder) == 0){
				item.type = 2; // folder
				item.path = folder;
				item.fuid = "";
				item.name = line.substr();
				cerr << "find " << item.name << "in folder " << folder << endl;
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		} else {
			// lowest level is file
			item.path = folder;
			string firstentry = line.substr(0, found);
			if (firstentry.compare(0, len, folder) == 0){
				item.type = 1; // file
				item.name = firstentry.substr();
				item.fuid = line.substr(found+1);
				cerr << "find " << item.name << "in folder " << folder << endl;
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		}
		start = pos+1;
		pos = rawdata.find("\n", start);
	}
}

/** Delete the specific file **/
Status StorageRepositoryImpl::DeleteFile(ServerContext* context, const GetFileRequest* request, AddFileResponse* response) {


	cerr<<"Received a request to delete file"<<endl;
	string user = request->username();
	// filename format: /folder(s)/filename
	string filename = request->filename();
	cerr<<"user="<<user<<" ; "<<"filename="<<filename<<endl;

	// update filelist in table
	std::vector<string> unmatched;
	// use deleteFolderHelper here, not parseFileList [we may have multiple entries for a single large file->duplicate filename].
	std::vector<filemeta> v = deleteFolderHelper(user, filename, unmatched, bigTablePtr);

	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}
	if (largestr.length() == 0) {
		bigTablePtr->dele(user, "filelist");
	} else {
		bigTablePtr->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	// delete file data from key-value storage
	for (auto it = v.begin(); it != v.end(); it++){
		struct filemeta m = *it;
		if (m.type == 1){
			cerr<<"find a matched file"<<endl;
			cerr<<"name="<<m.name<<"\t"<<"uid="<<m.fuid<<endl;
			bigTablePtr->dele(user, m.fuid);
			cerr<<"delete "<<m.name<<" from key-value storage"<<endl;
		}
	}
	cerr<<"deleted "<<filename<<", "<<"update filelist for "<<user<<endl;
	cerr << "------------------begin the table ---------------------------" << endl;
	bigTablePtr->print();
	cerr << "------------------end the table ---------------------------" << endl;

	return Status::OK;

}

/** Delete all files and subfolders under the specific folder, i.e. recursive deletion **/
Status StorageRepositoryImpl::DeleteFolder(ServerContext* context, const GetFileRequest* request, AddFileResponse* response) {
	
	cerr<<"Received a request to delete folder"<<endl;
	string user = request->username();
	// foldername format: /foldername/
	string foldername = request->filename();

	std::vector<string> unmatched;
	std::vector<filemeta> v = deleteFolderHelper(user, foldername, unmatched, bigTablePtr);
	// update filelist, or delete it if unmatched vector is empty
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}
	if (largestr.length() == 0) {
		bigTablePtr->dele(user, "filelist");
	} else {
		bigTablePtr->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	// delete file data from key-value storage
	for (auto it = v.begin(); it != v.end(); it++){
		struct filemeta m = *it;
		if (m.type == 1){
			cerr<<"name="<<m.name<<"\t"<<"uid="<<m.fuid<<" deleted "<<m.name<<" from key-value storage"<<endl;
			bigTablePtr->dele(user, m.fuid);
		}
	}
	cerr << "see the table ---------------------------" << endl;
	bigTablePtr->print();
	cerr << "see the table ---------------------------" << endl;

	return Status::OK;

}

/* Find meta information of a given file in filelist, return name and uid for each.
 * Duplicated name is allowed.
 * A large file may have multiple chunks in storage.
 */
vector<flistline> renameHelper(string user, string oldname, vector<string>& unmatched, BigTable* bigTablePtr) {

	std::vector<flistline> v;
	if (!bigTablePtr->exists(user, "filelist")) {
		return v;
	}
	string rawdata = bigTablePtr->get(user, "filelist");
	
	size_t len = oldname.length();
	size_t start = 0;
	size_t pos = rawdata.find("\n");
	string line = "";
	while(pos != std::string::npos){
		line = rawdata.substr(start, pos-start);
		struct flistline item;
		size_t found = line.find("\t");
		// lowest level is empty folder
		if (found == std::string::npos){

			if (line.compare(0, len, oldname) == 0){
				item.fuid = "";
				item.name = line.substr();
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		} else {
			// lowest level is file
			string firstentry = line.substr(0, found);
			if (firstentry.compare(0, len, oldname) == 0){
				item.name = firstentry.substr();
				item.fuid = line.substr(found+1);
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		}
		start = pos+1;
		pos = rawdata.find("\n", start);
	}
}

Status StorageRepositoryImpl::RenameFile(ServerContext* context, const RenameFileRequest* request, RenameFileResponse* response) {

	cerr<<"Received a request to rename file "<<request->oldname()<<endl;
	string user = request->username();
	// old file name format: /oldname
	// new file name format: /newname
	// both oldname and newname should contain full path to it
	string oldname = request->oldname();
	string newname = request->newname();

	std::vector<string> unmatched;
	std::vector<flistline> v = renameHelper(user, oldname, unmatched, bigTablePtr);
	// keep unmatched entries unchanged
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// change file name to new name
	for (auto it = v.begin(); it != v.end(); it++) {
		struct flistline line = *it;
		string s = newname + "\t" + line.fuid + "\n";
		largestr.append(s);
	}

	if (largestr.length() == 0) {
		bigTablePtr->dele(user, "filelist");
	} else {
		bigTablePtr->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	cerr << "see the table ---------------------------" << endl;
	bigTablePtr->print();
	cerr << "see the table ---------------------------" << endl;

	return Status::OK;
}

/* Rename a folder, and change path of all files/subfolders in it */
Status StorageRepositoryImpl::RenameFolder(ServerContext* context, const RenameFileRequest* request, RenameFileResponse* response) {

	cerr<<"Received a request to rename folder "<<request->oldname()<<endl;
	string user = request->username();
	// old folder name format: /oldname/
	// new folder name format: /newname/
	// both oldname and newname should contain full path to it
	string oldname = request->oldname();
	string newname = request->newname();

	std::vector<string> unmatched;
	std::vector<flistline> v = renameHelper(user, oldname, unmatched, bigTablePtr);
	// keep unmatched entries unchanged
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// change folder name to new name, attach anything after the folder as original
	for (auto it = v.begin(); it != v.end(); it++) {
		struct flistline line = *it;
		string s = "";
		if (line.name.length() > oldname.length()){
			string remain = line.name.substr(oldname.length());
			if (line.fuid.length() == 0) {
				s = newname + remain + "\n";
			} else {
				s = newname + remain + "\t" + line.fuid + "\n";
			}
		} else {
			s = newname + "\n";
		}
		largestr.append(s);
	}

	if (largestr.length() == 0) {
		bigTablePtr->dele(user, "filelist");
	} else {
		bigTablePtr->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	cerr << "see the table ---------------------------" << endl;
	bigTablePtr->print();
	cerr << "see the table ---------------------------" << endl;

	return Status::OK;
}

/* Change /oldfolder/file to /newfolder/file */
Status StorageRepositoryImpl::MoveFile(ServerContext* context, const MoveFileRequest* request, MoveFileResponse* response) {

	cerr<<"Received a request to move file "<<request->filename()<<"to folder "<<request->foldername()<<endl;
	string user = request->username();
	string filename = request->filename();
	string foldername = request->foldername();

	std::vector<string> unmatched;
	std::vector<flistline> v = renameHelper(user, filename, unmatched, bigTablePtr);
	// keep unmatched entries unchanged
	string largestr = "";
	for (auto it = unmatched.begin(); it != unmatched.end(); it++){
		largestr.append(*it);
		largestr.append("\n");
	}

	// change parent folder name to new name
	for (auto it = v.begin(); it != v.end(); it++) {
		struct flistline line = *it;
		size_t pos = line.name.find_last_of("/");
		string remain = line.name.substr(pos+1);
		string s = "";
		if (line.fuid.length() == 0) {
			s = foldername + remain + "\n";
		} else {
			s = foldername + remain + "\t" + line.fuid + "\n";
		}
		largestr.append(s);
	}

	if (largestr.length() == 0) {
		bigTablePtr->dele(user, "filelist");
	} else {
		bigTablePtr->put(user, "filelist", "txt", largestr.length(), largestr.c_str());
	}

	cerr << "see the table ---------------------------" << endl;
	bigTablePtr->print();
	cerr << "see the table ---------------------------" << endl;

	return Status::OK;
}

/** Find all files (or empty folders) begin with '/foldername/'.
 *  Unmatched entries are stored in umatched vector.
 */
vector<filemeta> StorageRepositoryImpl::deleteFolderHelper(string user, string folder, vector<string>& unmatched, BigTable* bigTablePtr) {

	std::vector<filemeta> v;
	if (!bigTablePtr->exists(user, "filelist")) {
		return v;
	}
	string rawdata = bigTablePtr->get(user, "filelist");

	size_t len = folder.length();
	size_t start = 0;
	size_t pos = rawdata.find("\n");
	string line = "";
	while(pos != std::string::npos){
		line = rawdata.substr(start, pos-start);
		cerr<<"read line: "<<line<<endl;
		struct filemeta item;
		size_t found = line.find("\t");
		// lowest level is empty folder
		if (found == std::string::npos){

			if (line.compare(0, len, folder) == 0){
				item.type = 2; // folder
				item.path = folder;
				item.fuid = "";
				item.name = line.substr();
				cerr << "find " << item.name << "in folder " << folder << endl;
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		} else {
			// lowest level is file
			item.path = folder;
			string firstentry = line.substr(0, found);
			if (firstentry.compare(0, len, folder) == 0){
				item.type = 1; // file
				item.name = firstentry.substr();
				item.fuid = line.substr(found+1);
				cerr << "find " << item.name << "in folder " << folder << endl;
				v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		}
		start = pos+1;
		pos = rawdata.find("\n", start);
	}

	return v;
}

/**
 * Given a user name and folder name, return a list of file names under the folder.
 * Folder input is '/' or '/foldername/', where '/' represents root directory.
 * Data format in "filelist":
 * /folder(s)/filename\tfuid\n
 * Returned vector does not contain duplicate folder names
 */
vector<filemeta> StorageRepositoryImpl::parseFileList(string user, string folder, vector<string>& unmatched, BigTable* bigTablePtr) {
	// cerr<<"parseFileList method is called"<<endl;
	std::vector<filemeta> v;
	if (!bigTablePtr->exists(user, "filelist")) {
		return v;
	}
	string rawdata = bigTablePtr->get(user, "filelist");
	std::set<string> foldernames;  // avoid duplicate folder names

	size_t len = folder.length();
	size_t start = 0;
	size_t pos = rawdata.find("\n");
	string line = "";
	while(pos != std::string::npos){
		line = rawdata.substr(start, pos-start);
		cerr<<"read line: "<<line<<endl;
		struct filemeta item;
		size_t found = line.find("\t");
		// lowest level is empty folder
		if (found == std::string::npos){
			if (line.compare(0, len, folder) == 0){
				item.type = 2; // folder
				item.path = folder;
				item.fuid = "";
				size_t t = line.find("/", len);
				if (t != string::npos){
					item.name = line.substr(len, t-len+1);
				} else {
					item.name = line.substr(len);
				}
				cerr<<"find in the folder "<<folder<<" : "<<item.name<<endl;
				// remove duplicate folder names
				std::set<string>::iterator it = foldernames.find(item.name);
				if (it == foldernames.end()){
					foldernames.insert(item.name);
					v.push_back(item);
				}
			} else {
				unmatched.push_back(line.substr());
			}
		} else {
			// lowest level is file
			item.path = folder;
			string firstentry = line.substr(0, found);
			if (firstentry.compare(0, len, folder) == 0){
				size_t t = firstentry.find("/", len);
				if (t != string::npos) {
					item.type = 2; // folder
					item.name = firstentry.substr(len, t-len+1);
					std::set<string>::iterator it = foldernames.find(item.name);
					if (it == foldernames.end()){
						foldernames.insert(item.name);
						v.push_back(item);
					}
					cerr<<"find in the folder "<<folder<<" : "<<item.name<<endl;
				} else {
					item.type = 1; // file
					item.name = firstentry.substr(len);
					item.fuid = line.substr(found+1);
					cerr<<"find in the folder "<<folder<<" : "<<item.name<<endl;
					// get file size and time
					if (bigTablePtr->exists(user, item.fuid)) {
						string filecontent = bigTablePtr->get(user, item.fuid);
						size_t idx = filecontent.find("\n");
						if (idx != string::npos){
							item.size = atoi(filecontent.substr(0, idx).c_str());
							size_t idx2 = filecontent.find("\n", idx+1);
							if (idx != string::npos){
								item.time = filecontent.substr(idx+1, idx2-idx-1);
							}
						}
					}
					v.push_back(item);
				}
				// v.push_back(item);
			} else {
				unmatched.push_back(line.substr());
			}
		}
		start = pos+1;
		pos = rawdata.find("\n", start);
	}

	return v;
}
