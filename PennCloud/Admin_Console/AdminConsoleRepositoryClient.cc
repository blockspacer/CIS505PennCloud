/*
 * EmailRespositoryClient.cc
 *
 *  Created on: Nov 11, 2017
 *      Author: cis505
 */


#include "AdminConsoleRepositoryClient.h"

BigTableRow AdminConsoleRepositoryClient::getRow(string user) {
	ClientContext context;

	GetUserRowRequest request;
	request.set_username(user);

	BigTableRow bigTableRow;

	cerr << "Calling getRow stub. " << endl;
	Status status = stub_->GetRowForUser(&context, request, &bigTableRow);
	cerr << "Done calling getRow stub." << endl;
	if (!status.ok()) {
		// log
		cerr << "getRow rpc failed." << endl;
	}
	else {
		cerr << "getRow success." << endl;
	}

	return bigTableRow;
}

vector<BigTableRow> AdminConsoleRepositoryClient::getAllRows() {
	ClientContext context;

	GetRowsRequest request;

	unique_ptr<ClientReader<BigTableRow>> reader(stub_->GetRows(&context, request));

	cerr << "AdminConsoleRepositoryClient: getAllRows stub call success";
	BigTableRow row;
	vector<BigTableRow> rows;
	while (reader->Read(&row)) {
		rows.push_back(row);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "GetAllRows rpc failed." << endl;
	}
	else {
		cerr << "GetAllRows rpc success." << endl;
	}

	return rows;
}

vector<BigTableRow> AdminConsoleRepositoryClient::getRows(int count, int offset) {
	ClientContext context;

	GetRowsRequest request;
	request.set_count(count);
	request.set_offset(offset);

	unique_ptr<ClientReader<BigTableRow>> reader(stub_->GetRows(&context, request));

	BigTableRow row;
	vector<BigTableRow> rows;
	while (reader->Read(&row)) {
		rows.push_back(row);
	}

	Status status = reader->Finish();

	if (!status.ok()) {
		cerr << "GetAllRows rpc failed." << endl;
	}

	return rows;
}

json fileToJson(const File &file) {
	json jsonStr;
	jsonStr["username"] = file.username();
	jsonStr["filename"] = file.filename();
	jsonStr["filetype"] = file.filetype();
	jsonStr["disk_filename"] = file.disk_filename();
	jsonStr["start"] = file.start();
	jsonStr["filesize"] = file.filesize();
	jsonStr["is_flushed"] = file.is_flushed();
	jsonStr["content"] = file.content();

	return jsonStr;
}

string AdminConsoleRepositoryClient::bigTableRowToJson(const BigTableRow &row) {
	json jsonStr;
	jsonStr["user"] = row.key();

	vector<json> columns;
	for (auto const& column : row.columns()) {
		json columnJson = fileToJson(column.second);
		columns.push_back(columnJson);
	}
	jsonStr["columns"] = json(columns);

	return jsonStr.dump();
}

string AdminConsoleRepositoryClient::bigTableRowsToJson(const vector<BigTableRow> &rows) {
	stringstream stream;

	stream << "[";
	for (int i = 0; i < rows.size(); i++) {
		stream << bigTableRowToJson(rows[i]);
		if (i != rows.size() - 1) {
			stream << ",";
		}
	}
	stream << "]";

	return stream.str();
}
