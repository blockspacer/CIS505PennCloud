/*
 * AdminConsoleRepositoryImpl.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */

#include "AdminConsoleRepositoryImpl.h"
using namespace std;

Status AdminConsoleRepositoryImpl::GetRowForUser(ServerContext* context, const GetUserRowRequest* request, BigTableRow* response) {
	// TODO: fill out method
	return Status::OK;
}

Status AdminConsoleRepositoryImpl::GetRows(ServerContext* context, const GetRowsRequest* request, ServerWriter<BigTableRow>* writer) {
	// TODO: fill out method
	request->count();
	request->offset();

	vector<string> users = bigTablePtr->get_users();

	for (string user : users) {
		BigTableRow row;
		row.set_key(user);
		unordered_map<string, Metadata> files = bigTablePtr->get_user_files(user);

		map<string, File> fileMap;
		for (auto file = files.begin(); file != files.end(); file++) {
			string fileName = file->first;
			Metadata metadata = file->second;

			File fileData;
			fileData.set_username(metadata.username);
			fileData.set_filename(metadata.filename);
			fileData.set_filetype(metadata.filetype);
			fileData.set_disk_filename(metadata.disk_filename);
			fileData.set_start(metadata.start);
			fileData.set_filesize(metadata.filesize);
			fileData.set_is_flushed(metadata.is_flushed);

			string value = bigTablePtr->get(user, fileName);
			fileData.set_content(value.substr(0, 5000));

			fileMap.insert(pair<string, File>(fileName, fileData));
		}

		row.mutable_columns()->insert(fileMap.begin(), fileMap.end());
		writer->Write(row);
	}

	return Status::OK;
}

