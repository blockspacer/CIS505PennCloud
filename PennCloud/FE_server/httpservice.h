/*
 * httpservice.h
 * PennCloud CIS 505
 * Author T01
 * Date: Nov 6 2017
 */
#ifndef HTTPSERVICE_H
#define HTTPSERVICE_H
#pragma once
#include "feconfig.h"
#include "../common/MasterServiceClient.h"
#include "StorageRepositoryClient.h"

using masterservice::WriteResponse;
using masterservice::AddressDTO;
using masterservice::MasterFileDTO;

typedef struct {
    string SessionID;
    string Domain;
    string Path;
    string Expire;
} Cookie;

//===================
// Http header class
//===================
class RespHeader{
public:
    string URI;
    string Http_v;
    string content_type;
    string connection_type;
    string cache_ctrl;
    string post_req;
    int content_len;
    bool has_cookie;
    Cookie cookie;
    RespHeader(): URI(""), Http_v("HTTP/1.1"), content_type(ctype_html), connection_type("keep-alive"),
    cache_ctrl(""), post_req(""), has_cookie(false),content_len(0) {};
};

//=================
//HttpService Class
//=================
class HttpService{
private:
    RespHeader *Rheader;
    string usrname;
    int connfd;      // file decriptor for worker threads
    bool set_cookie;
    bool db;
    bool is_login;
    string fpath;
    MasterServiceClient* masterServer;
    AdminConsoleRepositoryClient *adminClient;
    unordered_set<string> RouteSet;

    int HandleERR();
    //---GET---__
    int HandleGET(const string &Req);
    //---POST---
    int HandlePOST(const string &Req);
    int HandleLogin();
    int HandleDELETE();
    int HandleSHOWMail();
    int HandleRegister();
    int HandleCHPWD();
    int HandleUpload();
    int HandleSendEmail();
    int HandleDownload(const string &fileid);
    int HandleCFolder();    // create folder
    int HandleOFolder();    // open folder
    int HandleRename();
    int HandleMove();

    //---Common---
    int ParseReq(const string & Req);
    int ParseContentType(const string & action, string &content_type);
    int renderPage(const string & URL);
    int sendFile(const string &fileName);
    int SendNonHTML();
    int SendNonHTML(string body);
    int SetCookie(const string &username);
    void RemoveCookie();
    void ParseUAPOST(string &username, string &password);   // UAPOST stands for useraccount POST

    //---Helper---
    string gen_rand_string(size_t length); // helper function
    void urlDecode(char *dst, const char *src) ;
    void findAllFolders(unordered_map<string, vector <FileDTO> > &FolderMap, const string &path, const string& parentPath);

    //---Operations via gRPC---
    bool ValidateUser(const UserDTO &user);
    bool CreateUser(const UserDTO &user); // return false if user already exist
    bool ChangePwd(const string &oldpwd, const string &newpwd);  // return false if client typed the wrong old password
    int showMailul(stringstream &html_content); // add user's email to the inbox page
    int showFiles(stringstream &html_content, const unordered_map<string, vector <FileDTO> > &FolderMap);  // add user's files to the drive page
//    int showFileTree(stringstream &html_content); // add user's file tree to the drive page
    unordered_map<string, vector <FileDTO> > showFileTree(stringstream &html_content);
    void fileTreehelper(unordered_map<string, vector <FileDTO> > &FolderMap, stringstream &html_content, string foldername);

public:
    HttpService( int connfd, bool db, MasterServiceClient* masterServer);  // connfd is the fd for thread
    ~HttpService();
    int HandleCMD(const string &Cmd);
    int handleHttpRedirect(const string &cmd, const string &address);
    void addRoute(const string &route);

    void setAdminClient(AdminConsoleRepositoryClient* adminClientPtr) {
    	this->adminClient = adminClientPtr;
    }

    void setLogin(const bool &isLoggedIn) {
    	this->is_login = isLoggedIn;
    }
};

#endif