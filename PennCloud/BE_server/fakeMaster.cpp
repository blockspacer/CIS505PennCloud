//
// Created by cis505 on 12/1/17.
//
#include <grpc++/grpc++.h>
#include "../common/WriteServiceClient.h"
#include <thread>

int main(int argc, char *argv[]) {
    string address1 = "0.0.0.0:50051";
    WriteServiceClient client1(grpc::CreateChannel(address1, grpc::InsecureChannelCredentials()));
    vector<string> replicas;
    string address2 = "0.0.0.0:50052";
    replicas.push_back(address2);
//
//    client1.WriteCommand("rowkey1", "colkey1", "value1", WriteType::PUT, FileType::EMAIL, replicas, 1);
//
//
//    client1.WriteCommand("rowkey2", "colkey2", "value2", WriteType::PUT, FileType::EMAIL, replicas, 2);


    client1.WriteCommand("rowkey2", "colkey2", "value4", WriteType::PUT, FileType::EMAIL, replicas, 4);


    client1.WriteCommand("rowkey5", "colkey5", "value5", WriteType::PUT, FileType::EMAIL, replicas, 5);


//    WriteServiceClient client2(grpc::CreateChannel(address2, grpc::InsecureChannelCredentials()));
//
//    client2.WriteCommand("rowkey3", "colkey3", "value3", WriteType::PUT, FileType::EMAIL, replicas, 3);


}