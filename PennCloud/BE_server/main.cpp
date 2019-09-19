/*
 * main.cpp
 *
 *  Created on: Nov 10, 2017
 *      Author: cis505
 */

//#include "BigTable.h"


// -----------------------
// #include <grpc++/grpc++.h>
#include "EmailRepositoryImpl.h"

// using grpc::Server;
// using grpc::ServerBuilder;
using namespace std;



int main() {
	cout << "Start" << endl;
	BigTable * table =  new BigTable("server1");
    ImportMBOX(table,"mailboxes/linh.mbox", "linh");

	// char file1[] = "hi,";
	// table->put("Yeru", "file1.txt","email", strlen(file1), file1);

	// char file2[] = "world";
	// table->put("Yeru", "file2.txt","email", strlen(file2), file2);


	// char tmp[] = "nice to meet you";
	// table->put("Yeru", "file2.txt","email", strlen(tmp), tmp);
    // I insert into the test for WebmailService -- yezheng
	
    //--------------------------------
	//table->print();

	// table->dele("Yeru", "file3.txt");
	// table->dele("Yeru", "file2.txt");
	// cout << table->get("Yeru", "file2.txt") << endl;
	table->print();

	// table->dele("Yeru", "file3.txt");
	// table->dele("Yeru", "file2.txt");
	//cout << table->get("Yeru", "file2.txt") << endl;

	table->dele_disk();
	//table -> print();
	delete table;

	cout << "end" << endl;
	return 0;
}
