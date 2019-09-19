/*
 * httpservice.cc
 * PennCloud CIS 505
 * Author T01
 * Date: Nov 6 2017
 */
#include "httpservice.h"
#include "smtp_common.h"
//===========================
// HttpService Class
//===========================

bool External_SMTP(EmailDTO);

HttpService::HttpService(int connfd, bool db, MasterServiceClient* masterServer){
    this->db = db;
    this->connfd = connfd;
    this->set_cookie = false;
    this->Rheader = new RespHeader();
    this->usrname = "";
    this->fpath = "";
    this->is_login = false;
    this->masterServer = masterServer;
    this->adminClient = NULL;
    Rheader->cookie.SessionID = "";
    Rheader->cookie.Domain = "";
    Rheader->cookie.Path = "";
    Rheader->cookie.Expire = "";
    RouteSet.clear();
    RouteSet.insert("home");
    RouteSet.insert("inbox");
    RouteSet.insert("login");
    RouteSet.insert("drive");
    RouteSet.insert("register");
    RouteSet.insert("success");
    RouteSet.insert("failed");
}

HttpService::~HttpService(){
    delete Rheader;
}

int HttpService::HandleCMD(const string &Cmd){
    string HCmd;


    if(Cmd.length() >= 3){
        string::size_type pos = Cmd.find(' '); // find the position of '/', then parse the URI
        HCmd = Cmd.substr(0, pos);
        if(!HCmd.compare(Get)){
//            cerr<<Cmd<<endl;
            cerr<<"\n----------The above is GET REQUEST---------" <<endl;
            return HandleGET(Cmd);
        }else if(!HCmd.compare(Post)){
//            cerr<<Cmd<<endl;
            cerr<<"\n----------The above is POST REQUEST--------" <<endl;
            return HandlePOST(Cmd);
        }
    }
    return HandleERR();
}

int HttpService::handleHttpRedirect(const string &Cmd, const string &address){
	cerr << "Handling http redirect" << endl;
    string HCmd;
    if(Cmd.length() >= 3){
        string::size_type pos = Cmd.find(' '); // find the position of '/', then parse the URI
        HCmd = Cmd.substr(0, pos);

        ParseReq(Cmd);

        stringstream response;

        response << Rheader->Http_v << " 307 Temporary Redirect\r\n";
        response << "Location: " << address << "/" << Rheader->URI << "\r\n\r\n";

        string responseStr = response.str();
        cerr << responseStr << endl;
        auto len = responseStr.length();
        const char * msgbuf = responseStr.c_str();
        if(db) fprintf(stderr,"[%d] S: %s\n", connfd, msgbuf);
        send(connfd, msgbuf,len, 0);
        return 0;
    }

    return HandleERR();
}

void HttpService::addRoute(const string &route) {
	RouteSet.insert(route);
}


int HttpService::showMailul(stringstream &html_content){

	vector<MasterFileDTO> emailList = masterServer->GetEmails(usrname, FileType::EMAIL);

    vector<EmailDTO> emails;
    for (MasterFileDTO file : emailList) {
    	AddressDTO address = masterServer->RequestRead(usrname, file.uid());
    	EmailRepositoryClient EC(grpc::CreateChannel(address.address(), grpc::InsecureChannelCredentials()));

    	EmailDTO email = EC.getEmail(usrname, file.uid());
    	emails.push_back(email);
    }

    int i = 1;
    cerr<<"number of emails: " << emails.size()<<endl;
    for (EmailDTO email : emails) {
        const string &sender = email.sender();
        const string &message = email.message();
        const string &date = email.date();
        const string &title = email.subject();
        const string &uid = email.uid();

        html_content<< "<tr> <th scope='row' class='text-center'>" << i << "</th> <th class='text-center'>" << sender <<"</th>";
        html_content<< "<th class='text-center'>";
        html_content<< "<form class='email' action='"<< usrname << "/" << uid << "' method='POST'>";
        html_content<<  title << " ";
        html_content<< "<input type='submit' class ='btn btn-xs' name='method' value='SHOWMAIL'>";
        html_content<< "<input class='hidden' name='user' value='"<< usrname << "'>";
        html_content<< "<input class='hidden' name='uid' value='" << uid << "'>";
        html_content<< "</form> </th>  <th class='text-center'>"+ date + "</th> <th class='text-center'>";
        html_content<< "<form class='delete-form' action='/inbox/"<< usrname <<"' method='POST'>";
        html_content<< "<input type='submit' class ='btn btn-xs btn-danger' name='method' value='DELETE'>";
        html_content<< "<input class='hidden' name='user' value='"<< usrname << "'>";
        html_content<< "<input class='hidden' name='uid' value='" << uid << "'>";
        html_content<< "</form> </th> </tr>";
        i++;
	}
    return 0;
}

void HttpService::findAllFolders(unordered_map<string, vector <FileDTO> > &FolderMap, const string &path, const string &parentPath){
//    cerr<<"Parent Path is "<< parentPath<<endl;
    cerr<<"Calling findAllFolders on "<< parentPath + path<<endl;

    vector<AddressDTO> addresses = masterServer->RequestAddressesForFileList(usrname, FileType::FILE);

    vector<FileDTO> filelist;
    cerr << "Getting files from servers: " << endl;
    for (AddressDTO address : addresses) {
    	cerr << address.address() << endl;
    	grpc::ChannelArguments channelArgs;
    	channelArgs.SetMaxReceiveMessageSize(20 * 1024 * 1024);
    	StorageRepositoryClient SC(grpc::CreateCustomChannel(address.address(), grpc::InsecureChannelCredentials(), channelArgs));
    	vector<FileDTO> files = SC.getFileList(usrname, parentPath + path);

    	filelist.insert(filelist.end(), files.begin(), files.end());
    }

    // find all the folders
    for(FileDTO file: filelist){
        if(file.type() == "folder"){
            pair<string, vector<FileDTO> >  newEntry;
            vector<FileDTO > ffiles;
            newEntry.first = file.filename();
            newEntry.second = ffiles;
            FolderMap.insert( newEntry);
//            cerr<< "calling "<< path + newEntry.first + "/" <<endl;
            if(path != "/"){
//                findAllFolders(FolderMap, newEntry.first + "/", "/" + parentPath + path);
                findAllFolders(FolderMap, newEntry.first + "/", parentPath + path);
            } else {
                findAllFolders(FolderMap, newEntry.first + "/", path);
            }
        }
        string foldername = "/";
        if(parentPath + path != "/"){
            foldername = path.substr(0);
            foldername = foldername.substr(0,foldername.size() - 1);
        }

        auto entry = FolderMap.find(foldername);
        entry->second.push_back(file);
    }
}


unordered_map<string, vector <FileDTO> > HttpService::showFileTree(stringstream &html_content){
    unordered_map<string, vector <FileDTO> > FolderMap;  // <Key: folder | value: its files>
    // Add the root folder
    pair<string, vector<FileDTO> >  newEntry;
    vector<FileDTO > ffiles;
    newEntry.first = "/";
    newEntry.second = ffiles;
    FolderMap.insert(newEntry);

    // fill out the Folder Map
    findAllFolders(FolderMap, "/","");

    auto root = FolderMap.find("/");

    string foldername = "/";
    if(fpath != "/"){
        foldername = fpath.substr(1);
        foldername = foldername.substr(0,foldername.size() - 1);
        string::size_type pos = foldername.find("/");
        while(pos != string::npos){
            pos++;
            foldername = foldername.substr(pos);
            pos = foldername.find("/");
        }
    }
    //TODO: Ask which one would you prefer, show all path in the tree or the current path only
    fileTreehelper(FolderMap, html_content, foldername);   // show FileTree for current path only
//    fileTreehelper(FolderMap, html_content, "/");     // show FileTree for full path

    return FolderMap;
}

void HttpService::fileTreehelper(unordered_map<string, vector <FileDTO> > &FolderMap, stringstream &html_content, string foldername){
    auto root = FolderMap.find(foldername);
    cerr<< "---------------"<<endl;
    cerr<<"folder name is "<< foldername <<endl;
    cerr<< "---------------"<<endl;
    if(foldername != "/"){
        html_content << "<li><label class='tree-toggle nav-header selected'>"<< foldername<< "</label>";
        html_content << "<ul class='nav nav-list tree'>";
    }

    if (root != FolderMap.end()){
        for(auto f :root->second ){
            if(f.type() == "folder") {
//            cerr<<"finda Folder! foldername is:  "<<f.filename()<<endl;
                fileTreehelper(FolderMap, html_content, f.filename());
            } else {
//            html_content <<  "<li><a href='#'>"<<f.filename()<<"</a></li>";
                html_content<< " <li><a href='/drive/" << usrname << "/" << f.uid()<<"' type='submit' method='GET'";
                html_content<< " download='"<< f.filename() << "'>"<<f.filename() <<"</a> </li>";
            }
        }
    }


    if(foldername != "/"){
        html_content<<"</ul>";
        html_content<<"</li>";
    }
}


string findFullfolderName(string foldername, const unordered_map<string, vector <FileDTO> > & FolderMap){
    if(foldername == "/"){
        return foldername;
    }

    string fullname = foldername;
    string currentfolder = foldername;
    string parentfolder = "";
    for(auto entry : FolderMap){
        if(entry.first == currentfolder){
            continue;
        }
        parentfolder = entry.first;

        if(parentfolder != "/"){
            for(auto file : entry.second){
                if(file.filename() == currentfolder){
                    // filename is a parent folder of target folder
                    fullname = parentfolder + "/" + fullname;
                    currentfolder = parentfolder;
                    break;
                }
            }
        }
    }
    return "/" + fullname + "/";
}

int HttpService::showFiles(stringstream &html_content, const unordered_map<string, vector <FileDTO> > & FolderMap){
    cerr<<"current fpath is" <<fpath<<endl;

    vector<AddressDTO> addresses = masterServer->RequestAddressesForFileList(usrname, FileType::FILE);

    vector<FileDTO> filelist;
    cerr << "Getting files from servers: " << endl;
    for (AddressDTO address : addresses) {
    	cerr << address.address() << endl;
    	grpc::ChannelArguments channelArgs;
    	channelArgs.SetMaxReceiveMessageSize(20 * 1024 * 1024);
    	StorageRepositoryClient SC(grpc::CreateCustomChannel(address.address(), grpc::InsecureChannelCredentials(), channelArgs));
    	vector<FileDTO> files = SC.getFileList(usrname, fpath);

    	filelist.insert(filelist.end(), files.begin(), files.end());
    }

    int i = 1;
    cerr<<"number of Files: " << filelist.size()<<endl;
    for (FileDTO file : filelist) {
        const string &filename = file.filename();
        const string &filetype = file.type();
        const string &fileid = file.uid();
        const long &size = file.size();
        const string &time = file.time();
        const string &uid = file.uid();
        const string &filepath = fpath + filename;
        html_content<< "<tr>";
        html_content<< "<th scope='row' class='text-center'>"<< i <<"</th>";
        if(filetype != "folder"){
            html_content<< "<th class='text-center'>" << filename << "</th>";
            html_content<< "<th class='text-center'>" << filetype <<"</th>";
            // ------------- converting file size
            double printsize = size;  // unit bytes
            int size_unit_count = 0;
            string size_unit = " Bytes";
            while(printsize >= 1000){
                printsize /=1028;
                size_unit_count++;
            }
            if(size_unit_count == 3){
                size_unit = " GB";
            } else if (size_unit_count == 2){
                size_unit = " MB";
            } else if (size_unit_count == 1){
                size_unit = " KB";
            }
            // ------------- converting file size
            html_content<< "<th class='text-center'>" << setprecision(3)<< printsize<<size_unit<<"</th>";
            html_content<< "</th><th class='text-center'>";
            html_content<<"<div class='dropdown'>";
            html_content<< "<button class='btn btn-xs btn-info dropdown-toggle' type='button'";
            html_content<< "data-toggle='dropdown'>"<< "Move";
            html_content<< "<span class='caret'></span></button>";
            html_content<< "<ul class='dropdown-menu'>";

            int j = 0;
            html_content<< "<form id='file_move"<<i<<j<<"'";
            html_content<< "style='margin:auto;' class='delete-form' action='/drive/"<< usrname<< "' method='POST'>";
            html_content<< "<input class='hidden' name='method' value='MOVE'>";
            html_content<< "<input class='hidden' name='uid' value='" << fileid  << "'>";
            html_content<< "<input class='hidden' name='oldname' value='" << filepath  << "'>";
            html_content<< "<input class='hidden' name='newname' value='" << "/" << filename << "'>";
            html_content<< "</form>";
            html_content<< "<li> <a href=\"javascript:{}\" onclick=\"document.getElementById('file_move"<<i<<j<<"').submit();\" class='dropdown-item' >";
            html_content<< "/ </a></li>";
            j++;
            for(auto folder : FolderMap){
                string fullfoldername = findFullfolderName(folder.first, FolderMap);

                if(folder.first != "/" && folder.first != filename){
                    html_content<< "<form id='file_move"<<i<<j<<"'";
                    html_content<< "style='margin:auto;' class='delete-form' action='/drive/"<< usrname<< "' method='POST'>";
                    html_content<< "<input class='hidden' name='method' value='MOVE'>";
                    html_content<< "<input class='hidden' name='uid' value='" << fileid  << "'>";
                    html_content<< "<input class='hidden' name='oldname' value='" << filepath  << "'>";
                    html_content<< "<input class='hidden' name='newname' value='" << fullfoldername << filename << "'>";
                    html_content<< "</form>";
                    html_content<< "<li> <a href=\"javascript:{}\" onclick=\"document.getElementById('file_move"<<i<<j<<"').submit();\" class='dropdown-item' >";
                    html_content<< folder.first <<"</a></li>";
                    j++;
                }
            }
            html_content<<"</ul></div>";
            // Move Folder done------------------
            html_content<< "<th class='text-center'>";
            html_content<< "<div class='dropdown'>";
            html_content<< "<button class='btn btn-xs btn-warning dropdown-toggle' type='button'";
            html_content<< "data-toggle='dropdown'>"<< "Action";
            html_content<< "<span class='caret'></span></button>";
            html_content<< "<ul class='dropdown-menu'>";

            html_content<< "<form id='file_delete"<<i<<"'";
            html_content<< "style='margin:auto;' class='delete-form' action='/drive/"<< usrname<< "' method='POST'>";
            html_content<< "<input class='hidden' name='method' value='DELETE'>";
            html_content<< "<input class='hidden' name='user' value='"<< usrname << "'>";
            html_content<< "<input class='hidden' name='uid' value='" << filepath  << "'>";
            html_content<< "</form>";
            html_content<< "<li> <a href=\"javascript:{}\" onclick=\"document.getElementById('file_delete"<<i<<"').submit();\" class='dropdown-item' >";
            html_content<< "Delete</a></li>";
            html_content<< "<li><a href='/drive/" << usrname << "/" << fileid<<"' type='submit' method='GET'";
            html_content<< " class='dropdown-item' download='"<< filename << "'>Download</a></li>";
            html_content<< "<div class=\"dropdown-divider\"></div>";

//            html_content<<" <li><a href='#'data-toggle=\"modal\" data-target=\"#modalCName\"> Change Name" << "</a></li>";
            html_content<<" <li><a href='#'data-toggle=\"modal\" data-target=\"#modalCName"<< i<< "\">" << "Change Name" << "</a></li>";

            html_content<< "</th>";
            html_content<< "    <div class=\"modal fade\" id=\"modalCName"<< i<< "\" tabindex=\"-1\" role=\"dialog\" aria-labelledby=\"exampleModalLabel\" aria-hidden=\"true\">\n"
                    "        <div class=\"modal-dialog\" role=\"document\">\n"
                    "            <div class=\"modal-content\">\n"
                    "                <div class=\"modal-header\">\n"
                    "                    <h5 class=\"modal-title\" id=\"exampleModalLabel\">Change file name</h5>\n"
                    "                    <button type=\"button\" class=\"close\" data-dismiss=\"modal\" aria-label=\"Close\">\n"
                    "                        <span aria-hidden=\"true\">&times;</span>\n"
                    "                    </button>\n"
                    "                </div>\n"
                    "                <div class=\"modal-body\" style=\"text-align: center\">\n";
            html_content<< "<form   method= \"POST\" id='renameform"<< i << "'>\n";
            html_content<< "<input class='hidden' name='method' value='RENAME'>";
            html_content<< "<input class='hidden' name='uid' value='" << fileid  << "'>";
            html_content<< "<input class='hidden' name='oldname' value='" << filename  << "'>";
            html_content<< "<input type=\"text\" name=\"newname\" placeholder=\" new file name\">\n"
                    "                    </form>\n"
                    "                </div>\n"
                    "                <div class=\"modal-footer\">\n"
                    "                    <button type=\"button\" class=\"btn btn-secondary\" data-dismiss=\"modal\">Close</button>\n"
                    "                    <button type=\"submit\" form='renameform"<< i << "'class=\"btn btn-primary\">Save changes</button>\n"
                    "                </div>\n"
                    "            </div>\n"
                    "        </div>\n"
                    "    </div>";
            html_content<< "<th class='text-center'>" << time <<"</th>";
        } else {
            cerr<<file.filename() << endl;
            cerr<<"fpath is "<< fpath<< endl;
            html_content<< "<th class='text-center'> " ;
            html_content<< "<form id='ofolder"<< i<<"' action='/drive/"<< usrname <<filepath << "' method='POST'>";
            html_content<< "<input class='hidden' name='method' value='OFOLDER'>";
            html_content<< "<a href='javascript:{}' onclick=\"document.getElementById('ofolder"<< i<<"').submit();\">";
            html_content<< "<span class='folder'><i class='fa fa-folder-open-o fa-lg'></i> "<< filename << "</span> </th>";
            html_content<<  "</a>";
            html_content<< "</form>";
            html_content<< "<th class='text-center'>" << filetype <<"</th>";
            html_content<< "<th class='text-center'> - </th>";
            html_content<< "<th class='text-center'> - </th>";

//            // Move Folder
//            html_content<< "</th><th class='text-center'>";
//            html_content<<"<div class='dropdown'>";
//            html_content<< "<button class='btn btn-xs btn-info dropdown-toggle' type='button'";
//            html_content<< "data-toggle='dropdown'>"<< "Move";
//            html_content<< "<span class='caret'></span></button>";
//            html_content<< "<ul class='dropdown-menu'>";
//
//            int j = 0;
//            html_content<< "<form id='file_move"<<i<<j<<"'";
//            html_content<< "style='margin:auto;' class='delete-form' action='/drive/"<< usrname<< "' method='POST'>";
//            html_content<< "<input class='hidden' name='method' value='MOVE'>";
//            html_content<< "</form>";
//            html_content<< "<li> <a href=\"javascript:{}\" onclick=\"document.getElementById('file_move"<<i<<j<<"').submit();\" class='dropdown-item' >";
//            html_content<< "/ </a></li>";
//            j++;
//            for(auto folder : FolderMap){
//                string fullfoldername = findFullfolderName(folder.first, FolderMap);
//
//                if(folder.first != "/" && folder.first != filename){
////                    html_content<<"<li><a href='#'>" << folder.first << "</a></li>";
//                    html_content<< "<form id='file_move"<<i<<j<<"'";
//                    html_content<< "style='margin:auto;' class='delete-form' action='/drive/"<< usrname<< "' method='POST'>";
//                    html_content<< "<input class='hidden' name='method' value='MOVE'>";
//                    html_content<< "<input class='hidden' name='uid' value='" << fileid  << "'>";
//                    html_content<< "<input class='hidden' name='oldname' value='" << filepath + "/" << "'>";
//                    html_content<< "<input class='hidden' name='newname' value='" << fullfoldername  << "'>";
//                    html_content<< "</form>";
//                    html_content<< "<li> <a href=\"javascript:{}\" onclick=\"document.getElementById('file_move"<<i<<j<<"').submit();\" class='dropdown-item' >";
//                    html_content<< folder.first <<"</a></li>";
//                    j++;
//                }
//            }
//            html_content<<"</ul></div>";
            // Move Folder --- done--------------
            html_content<< "<th class='text-center'>";
            html_content<<"<div class='dropdown'>";
            html_content<< "<button class='btn btn-xs btn-warning dropdown-toggle' type='button'";
            html_content<< "data-toggle='dropdown'>"<< "Action";
            html_content<< "<span class='caret'></span></button>";
            html_content<< "<ul class='dropdown-menu'>";

            html_content<< "<form id='file_delete"<<i<<"'";
            html_content<< "style='margin:auto;' class='delete-form' action='/drive/"<< usrname<< "' method='POST'>";
            html_content<< "<input class='hidden' name='method' value='DELETE'>";
            html_content<< "<input class='hidden' name='user' value='"<< usrname << "'>";
            html_content<< "<input class='hidden' name='uid' value='" << filepath  + "/" << "'>";
            html_content<< "</form>";
            html_content<< "<li> <a href=\"javascript:{}\" onclick=\"document.getElementById('file_delete"<<i<<"').submit();\" class='dropdown-item' >";
            html_content<< "Delete</a></li>";
            html_content<< "<div class=\"dropdown-divider\"></div>";

            // TODO: enable change foldername once it get supported
//            html_content<<" <li><a href='#'data-toggle=\"modal\" data-target=\"#modalCName"<< i<< "\">" << "Change Name" << "</a></li>";
//            html_content<< "</th>";
//                        html_content<< "    <div class=\"modal fade\" id=\"modalCName"<< i<< "\" tabindex=\"-1\" role=\"dialog\" aria-labelledby=\"exampleModalLabel\" aria-hidden=\"true\">\n"
//                    "        <div class=\"modal-dialog\" role=\"document\">\n"
//                    "            <div class=\"modal-content\">\n"
//                    "                <div class=\"modal-header\">\n"
//                    "                    <h5 class=\"modal-title\" id=\"exampleModalLabel\">Change file name</h5>\n"
//                    "                    <button type=\"button\" class=\"close\" data-dismiss=\"modal\" aria-label=\"Close\">\n"
//                    "                        <span aria-hidden=\"true\">&times;</span>\n"
//                    "                    </button>\n"
//                    "                </div>\n"
//                    "                <div class=\"modal-body\">\n";
//            html_content<< "<form style=\"text-align: center\" method= \"POST\" id='renameform"<< i << "'>\n";
//            html_content<< "<input class='hidden' name='method' value='RENAME'>";
//            html_content<< "<input class='hidden' name='uid' value='" << fileid  << "'>";
//            html_content<< "<input class='hidden' name='oldname' value='" << filename  << "'>";
//            html_content<< "<input type=\"text\" name=\"newname\" placeholder=\"new file name\">\n"
//                    "                    </form>\n"
//                    "                </div>\n"
//                    "                <div class=\"modal-footer\">\n"
//                    "                    <button type=\"button\" class=\"btn btn-secondary\" data-dismiss=\"modal\">Close</button>\n"
//                    "                    <button type=\"submit\" form='renameform"<< i << "'class=\"btn btn-primary\">Save changes</button>\n"
//                    "                </div>\n"
//                    "            </div>\n"
//                    "        </div>\n"
//                    "    </div>";

            html_content<< "<th class='text-center'>" << time <<"</th>";
        }

        html_content<< "</tr>";

        i++;
	}
    return 0;
}

int HttpService::SendNonHTML(){
    string URL = Rheader->URI;
    // take off the headings from the URI : i.e /inbox/pcloud.css ----> pcloud.css
    string::size_type pos = URL.find('/');
    string::size_type lastpos = pos;
    while(pos != string::npos){
        lastpos = pos;
        URL = URL.substr(lastpos + 1);
        pos = URL.find('/');
    }

    string line;
    stringstream Response;
    stringstream content;
    ifstream myfile("views/" + URL);
    if(myfile.is_open()){
        while(getline(myfile,line)){
            content<<line << endl;
        }
    } else {
        return HandleERR();
    }
    string s_content = content.str();
    Rheader->content_len = (int) s_content.length();
    Response << Rheader->Http_v << " 200 OK\r\n";
    Response << "Content-type: " << Rheader->content_type  << "\r\n";
    Response << "Connection-type: " << Rheader->connection_type  << "\r\n";
    Response << "Content-length: " <<Rheader->content_len  << "\r\n";
    Response <<"\r\n" << s_content;
    myfile.close();

    string Res = Response.str();
    auto len = Res.length();
    const char * msgbuf = Res.c_str();
    if(db) fprintf(stderr,"[%d] S: %s\n", connfd, msgbuf);
    send(connfd, msgbuf,len, 0);
    return 0;

}

/**
 * Send an html response with the provided body.
 */
int HttpService::SendNonHTML(string body){
    string URL = Rheader->URI;
    // take off the headings from the URI : i.e /inbox/pcloud.js ----> pcloud.js
    string::size_type pos = URL.find('/');
    string::size_type lastpos = pos;
    while(pos != string::npos){
        lastpos = pos;
        URL = URL.substr(lastpos + 1);
        pos = URL.find('/');
    }
//    cerr<< "-------------SendNONHTML------- " <<endl;
//    cerr<< "URL is " <<  URL <<endl;
//    cerr<< "-------------SendNONHTML------- " <<endl;

    string line;
    stringstream Response;
    stringstream content;

    content << body;

    string s_content = content.str();
    Rheader->content_len = (int) s_content.length();
    Response << Rheader->Http_v << " 200 OK\r\n";
    Response << "Content-type: " << Rheader->content_type  << "\r\n";
    Response << "Connection-type: " << Rheader->connection_type  << "\r\n";
    Response << "Content-length: " <<Rheader->content_len  << "\r\n";
    Response <<"\r\n" << s_content;

    string Res = Response.str();
    auto len = Res.length();
    const char * msgbuf = Res.c_str();
    if(db) fprintf(stderr,"[%d] S: %s\n", connfd, msgbuf);
    send(connfd, msgbuf,len, 0);
    return 0;

}

int HttpService::renderPage(const string & URL){
    string line;
    stringstream Response;
    stringstream html_content;

    ifstream myfile;
    myfile.open("views/" + URL);
    ifstream header("views/header.html");
    ifstream footer("views/footer.html");
    if(myfile.is_open() && header.is_open() && footer.is_open()){
        //----------
        // html header
        //----------
        if(is_login){
             while(getline(header,line)){
                 string::size_type pos = line.find("<li><a href=\"/login\">Login</a></li>");
                 if(pos != std::string::npos){
                     html_content<<"<li><a href=\"/logout\">Logout</a></li>"<<endl;
                     getline(header,line);
                 } else {
                     html_content<<line;
                 }
            }
        }else {
            while(getline(header,line)){
                html_content<<line;
            }
        }

        if(!URL.compare("inbox.html")){
            //----------------
            // render inbox page
            //----------------
            while(getline(myfile,line)) {
                string::size_type pos = line.find("<tbody id=\"emails\">");
                if (pos != std::string::npos) {
                    html_content<<line;
                    showMailul(html_content);
                } else {
                    html_content << line;
                }
            }
        }else if (!URL.compare("home.html")){
            /*
             * render home page
             */
            if(is_login){

                while(getline(myfile,line)) {
                    html_content<< line;
                    string::size_type pos = line.find("<div class=\"routes\">");
                    if (line.find("<div class=\"routes\">") != std::string::npos) {
                        html_content<< "<h2> Hello " <<  usrname <<"</h2>";
                        html_content<< "<a href=\"/inbox/" +  usrname + "\"";
                        html_content<< "class=\"btn btn-md btn-success homebtn\"> Webmail </a>";
                        html_content<< "<a href=\"/drive/" + usrname + "\"" ;
                        html_content<<"class=\"btn btn-md btn-success homebtn\"> CloudDrive </a>";
                        html_content<< "<button class=\"chpwd btn btn-md btn-primary homebtn \"> Change pwd </button>";
                        html_content<< "<form style='display: none; margin-top: 10px;' class='pwdform' action='/home' method= 'POST'>";
                        html_content<< "<input class='hidden' name='method' value='CHPWD'>";
                        html_content<< "<input type='password' name='oldpwd' placeholder='old password'>";
                        html_content<< "<input type='password' name='newpwd' placeholder='new password'> ";
                        html_content<< "<input type='submit' class='btn btn-primary btn-md' value='Change password!'> </form>";
                    }
                }
                html_content<<"<ul class=\"slideshow\">\n"
                        "    <li></li>\n"
                        "    <li></li>\n"
                        "    <li></li>\n"
                        "    <li></li>\n"
                        "    <li></li>\n"
                        "</ul>";
            } else {
                while(getline(myfile,line)) {
                    html_content << line;
                }
            }
        } else if (!URL.compare("show.html")){
            /*
             * render show email page
             */
            string POSTReq = Rheader->post_req;
            // extract username and uid from port_req
            string username, uid;
            string::size_type pos = POSTReq.find("user=");
            string::size_type posend = POSTReq.find("&uid=");
            if(pos != string::npos && posend != string::npos){
                pos+=5;
                auto un_len = posend - pos;
                username = POSTReq.substr(pos,un_len);
                posend +=5;
                uid = POSTReq.substr(posend);
            }
            AddressDTO address = masterServer->RequestRead(usrname, uid);
            EmailRepositoryClient EC(grpc::CreateChannel(address.address(), grpc::InsecureChannelCredentials()));
            EmailDTO email = EC.getEmail(usrname, uid);
            const string &sender = email.sender();
            const string &title = email.subject();
            const string &message = email.message();
            const string &date = email.date();
            const string &mailuid = email.uid();

            while(getline(myfile,line)) {
                html_content<<line;
                string::size_type pos = line.find("<div class='FWD'>");
                if(pos != string::npos){
                    html_content<< "<form action='inbox/send' method='POST'>";
                    html_content<< "<div class='form-group'>";
                    html_content<< "<h5> FWD To</h5>";
                    html_content<< "<input type='email' class='form-control' name='mailTo' placeholder='name@example.com'> </div>";
                    html_content<< " <div class='form-group'>";
                    html_content<< " <h5>Email Title</h5>";
                    html_content<< " <input type='text' class='form-control' name='title'";
                    html_content<< " value='"<< "FWD:"<< title <<"'> </div>";  // preload title field
                    html_content<< "<div class='form-group'>";
                    html_content<< " <h5>Email Content</h5>";
                    html_content<< " <textarea class='form-control' name='content' rows='8'>";
                    html_content<<"\n-------- Forward Message --------"<< endl;
                    html_content<<"Subject: " << title<<endl;
                    html_content<<"Date: " << date;
                    html_content<<"From: " << sender<<endl;
                    html_content<<"To: " << usrname<<endl;
                    html_content<< endl;
                    html_content<<message ;
                    html_content<<"</textarea> </div>";

                } else {
                    pos = line.find("<div class='RPLY'>");
                    if(pos != string::npos){
                        html_content<< "<form action='inbox/send' method='POST'>";
                        html_content<< "<div class='form-group'>";
                        html_content<< "<h5> RPLY To</h5>";
                        html_content<< "<input type='email' class='form-control' name='mailTo' value='"<<sender<<"@penncloud.com'> </div>";
                        html_content<< " <div class='form-group'>";
                        html_content<< " <h5>Email Title</h5>";
                        html_content<< " <input type='text' class='form-control' name='title'";
                        html_content<< " value='"<< "RPLY:"<< title <<"'> </div>";  // preload title field
                        html_content<< "<div class='form-group'>";
                        html_content<< " <h5>Email Content</h5>";
                        html_content<< " <textarea class='form-control' name='content' rows='8'>";
                        html_content<< "\n\n"<<"On "<< date.substr(0,date.length() - 1) <<" , "<< sender <<" write:"<< endl;
                        html_content<<message ;
                        html_content<<"</textarea> </div>";

                    }else {
                        pos = line.find("emailcontent");
                        if (pos != std::string::npos) {
                            html_content << "<p> Sender : " << sender << "</p>";
                            html_content << "<p> Email Title : " << title << "</p>";
                            html_content << "<p> Date : " << date << "</p>";
                            html_content << "<p> MailUID : " << mailuid << "</p>";
                            html_content << "<p> Email Body: </p>";
                            html_content << "<textarea class='emailbody col-xs-12' row='8' readonly>" << message << "</textarea>";
                            html_content << "</div>";
                            html_content << "<div class='container'>";
                            html_content << "<a href='/inbox/" << usrname
                                         << "' class='showbackbtn btn btn-md btn-success'>Back to Webmail </a>";
//                            html_content<< "<button id='fwdbtn' class=' btn btn-md btn-primary '>FWD</button>";
                            html_content<< "<a id =\"fwdbtn\" href='#'data-toggle=\"modal\"  class=\"btn btn-md block-inline btn-primary\" data-target=\"#FWD\">FWD</a>";
                            html_content<< "<a id =\"replybtn\" href='#'data-toggle=\"modal\"  class=\"btn btn-md block-inline btn-primary\" data-target=\"#REPLY\">REPLY</a>";
                        }
                    }
                }
            }

            //-----------------------------REPLY-----------------------------
            html_content<< "    <div class=\"modal fade\" id=\"REPLY\" tabindex=\"-1\" role=\"dialog\" aria-labelledby=\"exampleModalLabel\" aria-hidden=\"true\">\n"
                   "        <div class=\"modal-dialog\" role=\"document\">\n"
                   "            <div class=\"modal-content\">\n"
                   "                <div class=\"modal-header\">\n"
                   "                    <h5 class=\"modal-title\" id=\"exampleModalLabel\">Reply Email</h5>\n"
                   "                    <button type=\"button\" class=\"close\" data-dismiss=\"modal\" aria-label=\"Close\">\n"
                   "                        <span aria-hidden=\"true\">&times;</span>\n"
                   "                    </button>\n"
                   "                </div>\n"
                   "                <div class=\"modal-body\">\n"
                   "                    <form action='inbox/send' method=\"POST\" id=\"replyEmail\">\n";
            html_content<< "<div class='form-group'>";
            html_content<< "<h5> RPLY To</h5>";
            html_content<< "<input type='email' class='form-control' name='mailTo' value='"<<sender<<"@penncloud.com'> </div>";
            html_content<< " <div class='form-group'>";
            html_content<< " <h5>Email Title</h5>";
            html_content<< " <input type='text' class='form-control' name='title'";
            html_content<< " value='"<< "RPLY:"<< title <<"'> </div>";  // preload title field
            html_content<< "<div class='form-group'>";
            html_content<< " <h5>Email Content</h5>";
            html_content<< " <textarea class='form-control' name='content' rows='8'>";
            html_content<< "\n\n"<<"On "<< date.substr(0,date.length() - 1) <<" , "<< sender <<" write:"<< endl;
            html_content<<message ;
            html_content<<"</textarea> </div>";
            html_content<<"</form>\n"
                   "                </div>\n"
                   "                <div class=\"modal-footer\">\n"
                   "                    <button type=\"button\" class=\"btn btn-secondary\" data-dismiss=\"modal\">Close</button>\n"
                   "                    <button type=\"submit\" form=\"replyEmail\" class=\"btn btn-primary\">Send</button>\n"
                   "                </div>\n"
                   "            </div>\n"
                   "        </div>\n"
                   "    </div>";

            //------------------------------ FWD-----------------------------
            html_content<< "    <div class=\"modal fade\" id=\"FWD\" tabindex=\"-1\" role=\"dialog\" aria-labelledby=\"exampleModalLabel\" aria-hidden=\"true\">\n"
                   "        <div class=\"modal-dialog\" role=\"document\">\n"
                   "            <div class=\"modal-content\">\n"
                   "                <div class=\"modal-header\">\n"
                   "                    <h5 class=\"modal-title\" id=\"exampleModalLabel\">FWD Email</h5>\n"
                   "                    <button type=\"button\" class=\"close\" data-dismiss=\"modal\" aria-label=\"Close\">\n"
                   "                        <span aria-hidden=\"true\">&times;</span>\n"
                   "                    </button>\n"
                   "                </div>\n"
                   "                <div class=\"modal-body\">\n"
                   "                    <form action='inbox/send' method=\"POST\" id=\"fwdEmail\">\n";
            html_content<< "<div class='form-group'>";
            html_content<< "<h5> FWD To</h5>";
            html_content<< " <div id=\"field\">\n"
                  "                                <input id=\"field1\" type=\"email\"  name=\"mailTo\" placeholder='name@example.com'>\n"
                  "                                <button id=\"b1\" class=\"btn btn-success add-more\"  type=\"button\">+</button>\n"
                  "                            </div>" ;
//            html_content<< "<input type='email' class='form-control' name='mailTo' placeholder='name@example.com'> </div>";
            html_content<< " <div class='form-group'>";
            html_content<< " <h5>Email Title</h5>";
            html_content<< " <input type='text' class='form-control' name='title'";
            html_content<< " value='"<< "FWD:"<< title <<"'> </div>";  // preload title field
            html_content<< "<div class='form-group'>";
            html_content<< " <h5>Email Content</h5>";
            html_content<< " <textarea class='form-control' name='content' rows='8'>";
            html_content<<"\n-------- Forward Message --------"<< endl;
            html_content<<"Subject: " << title<<endl;
            html_content<<"Date: " << date;
            html_content<<"From: " << sender<<endl;
            html_content<<"To: " << usrname<<endl;
            html_content<< endl;
            html_content<<message ;
            html_content<<"</textarea> </div>";
            html_content<<"</form>\n"
                   "                </div>\n"
                   "                <div class=\"modal-footer\">\n"
                   "                    <button type=\"button\" class=\"btn btn-secondary\" data-dismiss=\"modal\">Close</button>\n"
                   "                    <button type=\"submit\" form=\"fwdEmail\" class=\"btn btn-primary\">Send</button>\n"
                   "                </div>\n"
                   "            </div>\n"
                   "        </div>\n"
                   "    </div>";
        } else if (!URL.compare("drive.html")){
            /*
             * render Drive page
             */
            string f_root_token = "drive/" + usrname;
            string::size_type pos = Rheader->URI.find(f_root_token);
            if(pos != string::npos){
                pos += f_root_token.size();
                fpath = Rheader->URI.substr(pos) + "/";
//                cerr<< "file path is " << fpath<<endl;
            }
            unordered_map<string, vector <FileDTO> >  FolderMap;
            while(getline(myfile,line)) {
                string::size_type  pos = line.find("<div class='drivediv'>");
                if(pos != std::string::npos){
                    html_content << line;
                    html_content << "<form class = 'upload' action='"<< "/"+ f_root_token + fpath + "upload";
                    html_content << "' method='POST' enctype='multipart/form-data'>";
                } else {
                    pos = line.find("<ul class='nav nav-list tree bullets'>");
                    if(pos != std::string::npos){
                        html_content << line;
                         FolderMap = showFileTree(html_content);
                    } else{
                        pos = line.find("<tbody id='files'>");
                        if (pos != std::string::npos) {
                            html_content << line;
                            showFiles(html_content, FolderMap);
                        } else {
                            html_content << line;
                        }
                    }
                }
            }
            html_content<< "<h4 style='text-align:center;'> Your current path is "<< fpath<<"</h4>";

            //--- Add On Dec 15------
        } else if ( !URL.compare("success.html")) {
            if(is_login && usrname != ""){
                while(getline(myfile,line)){
                    string::size_type pos = line.find("href='/home'");
                    html_content<<line;
                    if(pos != string::npos){
                        html_content<< "<a href=\"/inbox/" +  usrname + "\"";
                        html_content<< "class=\"btn btn-md btn-success homebtn\"> Webmail </a>";
                        html_content<< "<a href=\"/drive/" + usrname + "\"" ;
                        html_content<<"class=\"btn btn-md btn-success homebtn\"> CloudDrive </a>";
                    }
                }

            }else{
                while(getline(myfile,line)){
                    html_content<<line;
                }

            }
            //--- Add On Dec 15------
        } else {
            while(getline(myfile,line)){
                html_content<<line;
            }
        }
        //----------
        // html footer
        //----------
        while(getline(footer,line)){
            html_content<<line;
        }
        string s_html_content = html_content.str();
        Rheader->content_len = (int) s_html_content.length();
        Response << Rheader->Http_v << " 200 OK\r\n";
        Response << "Content-type: " << Rheader->content_type  << "\r\n";
        Response << "Connection-type: " << Rheader->connection_type  << "\r\n";
        Response << "Content-length: " <<Rheader->content_len  << "\r\n";
        if(set_cookie){
            if(Rheader->cookie.Expire.compare("")){
                Response << "Set-Cookie: "<< Rheader->cookie.SessionID<<";"<<Rheader->cookie.Expire <<endl;
            }else {
                Response << "Set-Cookie: "<< Rheader->cookie.SessionID<<endl;
            }
        }
        Response <<"\r\n" << s_html_content;
        myfile.close();
        header.close();
        footer.close();
    }else{
        return HandleERR();
    }

    string Res = Response.str();
    auto len = Res.length();
    const char * msgbuf = Res.c_str();
    if(db) fprintf(stderr,"[%d] S: %s\n", connfd, msgbuf);
    send(connfd, msgbuf,len, 0);
    return 0;
}

int HttpService::sendFile(const string &fileName){
    string line;
    stringstream Response;
    stringstream fileContents;

    ifstream myfile;
    myfile.open("views/" + fileName);
    if(myfile.is_open()) {

    	while(getline(myfile,line)) {
    		fileContents << line << endl;
    	}

    	string body = fileContents.str();
    	Rheader->content_len = (int) body.length();
    	Response << Rheader->Http_v << " 200 OK\r\n";
    	Response << "Content-type: " << Rheader->content_type  << "\r\n";
    	Response << "Connection-type: " << Rheader->connection_type  << "\r\n";
    	Response << "Content-length: " <<Rheader->content_len  << "\r\n";
    	Response <<"\r\n" << body;

    	myfile.close();
    }
    else{
        return HandleERR();
    }

    string Res = Response.str();
    auto len = Res.length();
    const char * msgbuf = Res.c_str();
    if(db) fprintf(stderr,"[%d] S: %s\n", connfd, msgbuf);
    send(connfd, msgbuf,len, 0);
    return 0;
}

int HttpService::HandleGET(const string &Req){
    ParseReq(Req);
    ParseContentType(Rheader->URI, Rheader->content_type);

    string rootURL = Rheader->URI;
    string nonHTML = "";
    //-------
    // cut off the specific routes such as
    // treat /home/user_id as /home
    //-------
    string::size_type pos = rootURL.find("/");
    if(pos != std::string::npos){
        if(Rheader->content_type == ctype_html){
            rootURL = rootURL.substr(0, pos);
        }
    }
    auto got = RouteSet.find(rootURL);
    if(got == RouteSet.end()){
        // the request is not a route, but might be .js or .css etc
        //handleURI
        if(is_login){
            if(Rheader->URI.compare("logout") == 0) {
                RemoveCookie();
                is_login = false;
                //TODO-----------------
                // Now just trivially render home page
                // Should redirect instead for full impl
                //---------------------
                return renderPage("home.html");
            }
            else if (Rheader->URI.substr(0, 12).compare("bigTableData") == 0 && adminClient != NULL) {
            	int pos = Rheader->URI.find('/');

            	vector<BigTableRow> rows;
            	if (pos != string::npos) {
            		string address = Rheader->URI.substr(pos + 1);
            		if (address.size() > 0) {
            			cerr << "Gettting big table data from " << address << endl;
            			cerr << "Getting big table data for admin console" << endl;
            			grpc::ChannelArguments channelArgs;
            			channelArgs.SetMaxReceiveMessageSize(-1);
            			AdminConsoleRepositoryClient adminClient(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), channelArgs));
            			rows = adminClient.getAllRows();
            		}
            		else {
            			// TODO: Compile the whole big table
            			cerr << "Compiling the whole big table" << endl;
            			vector<AddressDTO> primaryAddresses = masterServer->RequestAddressesForAllPrimaryServers();

            			cerr << "There are " << primaryAddresses.size() << " primary servers" << endl;
            			map<string, BigTableRow> rowMap;
            			for(AddressDTO &primary : primaryAddresses) {
            				cerr << "Getting complete big table data for admin console. Querying " << primary.address() << endl;
            				grpc::ChannelArguments channelArgs;
            				channelArgs.SetMaxReceiveMessageSize(-1);
            				AdminConsoleRepositoryClient adminClient(grpc::CreateCustomChannel(primary.address(), grpc::InsecureChannelCredentials(), channelArgs));
            				vector<BigTableRow> newRows = adminClient.getAllRows();

            				for (BigTableRow &row : newRows) {
            					if (rowMap.find(row.key()) != rowMap.end()) {
            						// Add to this users data
            						for (auto column : row.columns()) {
            							rowMap[row.key()].mutable_columns()->insert(column);
            						}
            					}
            					else {
            						// Add this user to the row map
            						rowMap[row.key()] = row;
            					}
            				}
            			}
            			for (auto &entry : rowMap) {
            				rows.push_back(entry.second);
            			}
            		}
            	}
            	else {
            		cerr << "No address for getting big table data" << endl;
            	}
            	cerr << "There are " << rows.size() << " rows in the big table" << endl;


            	return SendNonHTML(adminClient->bigTableRowsToJson(rows));
            }
            else {
                return SendNonHTML();
            }
        } else {
            return SendNonHTML();
        }
    }else {
        // the request is a valid URL
        if (is_login) {
            //check if it's a download request
            string fullURL = Rheader->URI;
            string token = usrname + "/";
            string drivedownload_token = "drive/" + usrname + "/";
            string file_id;
            string::size_type pos = fullURL.find(drivedownload_token);

            if(pos != string::npos){
                // this is a download GET req
                pos = fullURL.find(token);
                if(pos != string::npos){
                    pos += token.size();
                    file_id = fullURL.substr(pos);
                }
                cerr << "Download req!!" <<file_id << endl;
                HandleDownload(file_id);
                return renderPage("drive.html");
            }
            //-------------------------------
            // non download GET req
            return renderPage(*got + ".html");
        } else {
            //TODO-----------------
            // Now just trivially render to login page
            // Should redirect instead for full impl
            //---------------------
            if(*got == "home" || *got == "register"){
                return renderPage(*got + ".html");
            }
            return renderPage("login.html");
        }
    }
}


int HttpService:: HandleDownload(const string &fileid){
    string line;
    stringstream Response;
    cerr << "HandleDownload(): fileid: '" << fileid << "'" << endl;
    AddressDTO address = masterServer->RequestRead(usrname, fileid);
    grpc::ChannelArguments channelArgs;
    channelArgs.SetMaxReceiveMessageSize(20 * 1024 * 1024);
    StorageRepositoryClient SC(grpc::CreateCustomChannel(address.address(), grpc::InsecureChannelCredentials(), channelArgs));

    FileDTO file = SC.getFile(usrname,fileid);
    string filename = file.filename();
    string s_html_content = file.data();
    cerr<< "FRom big table size"<<file.data().size() << " converted size is " << s_html_content.size()<<endl;

    Rheader->content_len = (int) s_html_content.length();
    Rheader->content_type = ctype_plain;
    Response << Rheader->Http_v << " 200 OK\r\n";
    Response << "Content-type: " << Rheader->content_type  << "\r\n";
    Response << "Connection-type: " << Rheader->connection_type  << "\r\n";
    Response << "Content-length: " <<Rheader->content_len  << "\r\n";

    Response <<"\r\n" << s_html_content;

    string Res = Response.str();
    auto len = Res.length();
    const char * msgbuf = Res.c_str();
    if(db) fprintf(stderr,"[%d] S: %s\n", connfd, msgbuf);
    send(connfd, msgbuf,len, 0);
    return 0;
}

// get the URI and Http_v
int HttpService::ParseReq(const string &Req){
    // First check if the user has a cookie
    string::size_type pos = Req.find("Cookie:");
    if(pos != std::string::npos){
        //found a Cookie , set is_login
        is_login = true;
        //get username
        pos += 8;
        string cookieline = Req.substr(pos);
        string::size_type endpos = cookieline.find('=');
        this->usrname = cookieline.substr(0,endpos);
    }

    // find the position of '/', then parse the URI
    pos = Req.find('/');
    string req_content = Req.substr(pos);
    // get URI and HTTP version, so Parse the first line : i.e. GET /home HTTP/1.1
    stringstream ss(req_content);
    vector<string> reqs;
    string data;
    getline(ss, data,'\r');
    stringstream fl(data);
    while(getline(fl, data, ' ')){
        reqs.push_back(data);
    }

    if(reqs.size() == 2){
        Rheader->URI = reqs[0].substr(1);
        Rheader->Http_v = reqs[1];
        return 0;
    } else {
        return 1;
    }

    // TODO: -------------------
    // Then parse Connection-type: , Cache-Control
}

int HttpService::ParseContentType(const string & URI, string &content_type){
    stringstream ss(URI);
    string data;
    vector<string> datas;
    datas.clear();
//    cerr<< "URI -------------"<< endl;
//    cerr<< Rheader->post_req << endl;
//    cerr<< "URI -------------"<< endl;
    while(getline(ss, data, '.')){
        datas.push_back(data);
    }
    if(datas.size()==2){
        if (!datas[1].compare("css")){
            content_type = ctype_css;
        } else if (!datas[1].compare("jpg")){
            content_type = ctype_jpg;
        } else if (!datas[1].compare("js")){
            content_type = ctype_js;
        } else if (!datas[1].compare("png")){
            content_type = ctype_png;
        } else if (!datas[1].compare("jpeg")){
            content_type = ctype_jpeg;
        }
    } else if (datas.size()== 1){
        // here we should check the routes
        content_type = ctype_html;
    } else{
        return 1;
    }
    return 0;
}

int HttpService::HandlePOST(const string &Req){
    cerr<< " before ParseReq----"<<endl;
    ParseReq(Req);
    string POSTmethod;
    //get to \r\n\r\n and load the POST req
    string::size_type pos = Req.find(http_cmd_term);
    if(pos != string::npos){
        Rheader->post_req = Req.substr(pos+4);
    }
    pos = Rheader->post_req.find('&');
    // get the POST method
    if(pos != string::npos){
        POSTmethod = Rheader->post_req.substr(0, pos);
    } else {
        POSTmethod = Rheader->post_req;
    }

    if(Rheader->URI =="login"){
        return HandleLogin();
    } else if(Rheader->URI == "register"){
        return HandleRegister();
    } else if(Rheader->URI.find("inbox/send") != string::npos){
        return HandleSendEmail();
    } else if(Rheader->URI.find("/upload") != string::npos){
        cerr<< " before HandleUpload----"<<endl;
        return HandleUpload();
    } else if (Rheader->URI == "kill" && adminClient != NULL) {
    	// TODO: handle kill request for admin console
    	// TODO: Add handling for the specific client to send the kill to

    	cerr << "Kill client request body: \"" << Rheader->post_req << "\"" << endl;
    	HeartbeatServiceClient client(grpc::CreateChannel(Rheader->post_req, grpc::InsecureChannelCredentials()));

    	client.killServer();

    	json response;
    	response["success"] = true;

    	return SendNonHTML(response.dump());
    } else {
        if(POSTmethod == "method=DELETE"){
            // DELETE request
            return HandleDELETE();
        } else if(POSTmethod == "method=SHOWMAIL") {
            // SHOW the clicked Email content
            return HandleSHOWMail();
        } else if(POSTmethod == "method=CHPWD") {
            // Change the user's password
            return HandleCHPWD();
        } else if(POSTmethod == "method=CFOLDER"){
            return HandleCFolder();
        } else if (POSTmethod == "method=OFOLDER"){
            return HandleOFolder();
        } else if (POSTmethod == "method=RENAME"){
            return HandleRename();
        } else if (POSTmethod == "method=MOVE") {
            return HandleMove();
        } else {
            return HandleERR();
        }
    }
}

int HttpService::HandleMove(){
    string fileuid;
    string file_oldname;
    string file_newname;
    string fidtoken = "uid=";
    string foldnametoken = "oldname=";
    string fnewnametoken = "newname=";

    string::size_type pos = Rheader->post_req.find(fidtoken);
    string::size_type posmid = Rheader->post_req.find(foldnametoken);
    string::size_type posend = Rheader->post_req.find(fnewnametoken);
    if(pos != string::npos && posend != string::npos && posmid != string::npos){
        pos += fidtoken.size();
        string::size_type len = posmid - pos - 1;
        fileuid = Rheader->post_req.substr(pos,len);

        posmid += foldnametoken.size();
        len = posend - posmid - 1;

        file_oldname = Rheader->post_req.substr(posmid,len);
        char * dest = (char * )malloc( file_oldname.length()+ 1);
        urlDecode(dest, file_oldname.c_str());
        file_oldname = dest;
        free(dest);

        cerr<< "------------- file oldname is " << file_oldname << endl;

        posend += foldnametoken.size();
        file_newname = Rheader->post_req.substr(posend);
        char * dest2 = (char * )malloc( file_newname.length()+ 1);
        urlDecode(dest2, file_newname.c_str());
        file_newname = dest2;
        free(dest2);
        cerr<< "------------- file newname is " << file_newname << endl;
    }
    RenameFileRequest request;
    request.set_username(usrname);
    request.set_oldname(file_oldname);
    request.set_uid(fileuid);
    request.set_newname(file_newname);

    WriteResponse response = masterServer->RequestWrite(usrname, request.uid(), request.SerializeAsString(), WriteType::RENAME, FileType::FILE);

    if(response.success()){
        return renderPage("success.html");
    }

    return renderPage("failed.html");
}

int HttpService::HandleRename(){
    string fileuid;
    string file_oldname;
    string file_newname;
    string fidtoken = "uid=";
    string foldnametoken = "oldname=";
    string fnewnametoken = "newname=";

    string f_root_token = "drive/" + usrname;
    string::size_type pos_path = Rheader->URI.find(f_root_token);
    if(pos_path != string::npos){
        pos_path += f_root_token.size();
        fpath = Rheader->URI.substr(pos_path) + "/";
                cerr<< "file path is " << fpath<<endl;
    }
    string::size_type pos = Rheader->post_req.find(fidtoken);
    string::size_type posmid = Rheader->post_req.find(foldnametoken);
    string::size_type posend = Rheader->post_req.find(fnewnametoken);
    if(pos != string::npos && posend != string::npos && posmid != string::npos){
        pos += fidtoken.size();
        string::size_type len = posmid - pos - 1;
        fileuid = Rheader->post_req.substr(pos,len);

        posmid += foldnametoken.size();
        len = posend - posmid - 1;
        file_oldname = fpath + Rheader->post_req.substr(posmid,len);
        cerr<< "------------- file oldname is " << file_oldname << endl;

        posend += foldnametoken.size();
        file_newname = fpath + Rheader->post_req.substr(posend);
        cerr<< "------------- file newname is " << file_newname << endl;
    }

    RenameFileRequest request;
    request.set_username(usrname);
    request.set_oldname(file_oldname);
    request.set_uid(fileuid);
    request.set_newname(file_newname);

    WriteResponse response = masterServer->RequestWrite(usrname, request.uid(), request.SerializeAsString(), WriteType::RENAME, FileType::FILE);

    if(response.success()){
        return renderPage("success.html");
    }

    return renderPage("failed.html");
}

int HttpService::HandleOFolder() {
    cerr<<"Open Folder!!"<<endl;
    return renderPage("drive.html") ;
}

int HttpService::HandleCFolder() {
    cerr<<"ADD folder!" << endl;
    string ftoken = "foldername=";
    string::size_type pos = Rheader->post_req.find(ftoken);
    string foldername = "";
    if(pos != string::npos){
        pos += ftoken.size();
        foldername = Rheader->post_req.substr(pos);
    }else{
        return renderPage("failed.html");
    }

    string token2 = "drive/" + usrname;
    string::size_type  pos_2= Rheader->URI.find(token2);

    string fullfoldername = "";
    if( pos_2 != string::npos){
        pos_2 += token2.size();
        fullfoldername = Rheader->URI.substr(pos_2) +"/"+ foldername;
        cerr<< "full name of the file is "<<fullfoldername<<endl;
    } else {
        return renderPage("failed.html");
    }
    fullfoldername += "/";

    // call rpc and AddFolder
    FileDTO file;
    file.set_type("folder");
    file.set_owner(usrname);
    file.set_filename(fullfoldername);

    cerr<<"fullfolder name is ----------------"<<fullfoldername<<endl;
    masterServer->RequestWrite(usrname, fullfoldername, file.SerializeAsString(), WriteType::PUT, FileType::FOLDER);

    return renderPage("success.html");
}



int HttpService::HandleUpload(){

    string line, splitline, filename;
    stringstream ss, filecontent;

    // first get filename:
    ss<< Rheader->post_req;
    getline(ss,splitline); // load the splitline
    getline(ss,line); // this is the Content-Disposition line
    const string token="filename=";
    string::size_type pos = line.find(token);
    if(pos != string::npos){
        pos += token.size() + 1;
        filename = line.substr(pos);
    }
    string::size_type len = filename.size() -2;
    filename = filename.substr(0, len);

    // gets to the start of the content
    pos = Rheader->post_req.find(http_cmd_term);
    string post_content = Rheader->post_req.substr(pos);
    pos += http_cmd_term.size();
    string::size_type endpos = post_content.find(splitline);

    long cont_len = endpos - http_cmd_term.size() - 2;
    if(pos != string::npos && endpos != string::npos){
        post_content = Rheader->post_req.substr(pos, cont_len);
    }

    string data =  post_content;
    long long size = data.size();

    //TODO: write a helper functoin to wrap this get fullfilename
    string token1 = "/upload";
    string token2 = "drive/" + usrname + "/";
    pos = Rheader->URI.find(token1);
    string::size_type  pos_2= Rheader->URI.find(token2);

    string fullfilename = "";
    if(pos != string::npos && pos_2 != string::npos){
        pos_2 += token2.size();
        fullfilename = "/" + Rheader->URI.substr(pos_2, pos + 1 - pos_2) + filename;
        cerr<< "full name of the file is "<<fullfilename<<endl;
    } else {
        return renderPage("failed.html");
    }


    auto now = std::chrono::system_clock::now();
    time_t timen = std::chrono::system_clock::to_time_t(now);
    string date = ctime(&timen);

    FileDTO file;

    file.set_filename(fullfilename);
    file.set_data(data);
    file.set_owner(usrname);
    file.set_time(date.substr(0, date.size() - 1));

    // generate uid
    string filedata = file.data();
    unsigned char digestbuf[2*MD5_DIGEST_LENGTH+1];
    computeDigest(filedata.c_str(), filedata.length(), digestbuf);
    char BUFF[32];
    char MD5_id[2*MD5_DIGEST_LENGTH+1];
    memset(MD5_id, 0, sizeof(MD5_id));
    for (int j=0; j < MD5_DIGEST_LENGTH; j++){
        sprintf(BUFF, "%02x", digestbuf[j]);
        strcat(MD5_id, BUFF);
    }

    file.set_uid(string(MD5_id));
	file.set_size(filedata.length());

    //-----------
	if (filename.size() >0 && data.size() > 0) {

		WriteResponse response = masterServer->RequestWrite(usrname, file.uid(), file.SerializeAsString(), WriteType::PUT, FileType::FILE);

		if(response.success()){
			return renderPage("success.html");
		}
	}
    return renderPage("failed.html");
}

int HttpService::HandleCHPWD(){
    cerr<<"CHANGE pwd!!"<<endl;
    const string opwde = "oldpwd=";
    const string npwde = "newpwd=";
    string oldpwd, newpwd, buf;

    string::size_type pos,posend;

    // parse old password
    pos = Rheader->post_req.find(opwde);
    pos +=(opwde).length();
    posend =  Rheader->post_req.find(npwde);
    oldpwd = Rheader->post_req.substr(pos,posend - pos - 1);

    // parse new password
    pos = Rheader->post_req.find(npwde);
    pos +=(npwde).length();
    newpwd = Rheader->post_req.substr(pos);

    if (ChangePwd(oldpwd,newpwd)){
        return renderPage("success.html");
    }
    return renderPage("failed.html");
}

bool HttpService::ChangePwd(const string &oldpwd, const string &newpwd){
	AddressDTO address = masterServer->RequestRead(usrname, "password");

	UserRepositoryClient UC(grpc::CreateChannel(address.address(), grpc::InsecureChannelCredentials()));
    UserDTO olduser = UC.getUser(usrname);
    cerr<< "real oldpwd = "<<olduser.password() << " given oldpwd = "<<oldpwd<<" newpwd = \""<<newpwd <<"\""<<endl;
    if(olduser.password() == oldpwd){
        UserDTO newuser;
        newuser.set_password(newpwd);
        newuser.set_username(usrname);

        WriteResponse createResponse = masterServer->RequestWrite(usrname, "password", newpwd, WriteType::PUT, FileType::PASSWORD);
        return createResponse.success();
    }
    return false;
}

bool HttpService::ValidateUser(const UserDTO &user){
	AddressDTO address = masterServer->RequestRead(user.username(), "password");
    if(address.address()==""){
        return false;
    }

	UserRepositoryClient UC(grpc::CreateChannel(address.address(), grpc::InsecureChannelCredentials()));
	UserDTO getuser= UC.getUser(user.username());
    cerr<<getuser.password() << "    the given pwd = " << user.password()<<endl;
    return getuser.password() == user.password();
}

bool HttpService::CreateUser(const UserDTO &user){
    //first check if user already exist
	AddressDTO address = masterServer->RequestRead(user.username(), "password");

	if (address.address() != "") {
		// User already exists
		return false;
	}

    cerr << "Creating user" << endl;
    WriteResponse createResponse = masterServer->RequestWrite(user.username(), "password", user.password(), WriteType::PUT, FileType::PASSWORD);
    return createResponse.success();
}
int HttpService::HandleDELETE() {
     cerr << Rheader->URI << endl;

     // parse the uid from Rheader->post_req;
     const string uide = "uid=";
     string uid;
     string::size_type pos;
     pos = Rheader->post_req.find(uide);
     pos +=(uide).length();
     uid = Rheader->post_req.substr(pos);

     char * dest = (char * )malloc( uid.length()+ 1);
     urlDecode(dest, uid.c_str());
     uid = dest;
     free(dest);

     pos = Rheader->URI.find(("inbox"));
     if(pos != string::npos){
     	WriteResponse response = masterServer->RequestWrite(usrname, uid, "", writeservice::WriteType::DELETE, FileType::EMAIL);
     	// TODO: Check response for success
         return renderPage("inbox.html");
     } else {
         cerr<< "delete file is "<< uid <<endl;
         if(uid[uid.size() -1 ] == '/'){
             cerr<< "DEL folder" <<endl;
             WriteResponse response = masterServer->RequestWrite(usrname, uid, "", WriteType::DELETE, FileType::FOLDER);
             // TODO: Check response for success
         } else {
             cerr<< "DEL file" <<endl;
             WriteResponse response = masterServer->RequestWrite(usrname, uid, "", WriteType::DELETE, FileType::FILE);
             // TODO: Check response for success
         }
         return renderPage("drive.html");
     }
}

int HttpService::HandleSHOWMail() {
    // Test on getting only one email
    return renderPage("show.html");
}

int HttpService::HandleSendEmail(){
    cerr<<"Send Email!"<<endl;

    auto now = std::chrono::system_clock::now();
    time_t timen = std::chrono::system_clock::to_time_t(now);
    string date = ctime(&timen);

    // decode the POST request
    string post_req_buf = Rheader->post_req;
    char * dest = (char * )malloc( post_req_buf.length()+ 1);
    urlDecode(dest, post_req_buf.c_str());
    string DecodedReq = dest;
    free(dest);

    // parse receiver, subject and message
    string::size_type pos = 0;
    string::size_type posend = 0;
    string emailTo = DecodedReq.substr(0, pos);
    string buf;
    vector<string> SendEmailInfoList;
    while((pos = DecodedReq.find("&")) != string::npos){
        buf = DecodedReq.substr(0, pos);
        SendEmailInfoList.push_back(buf);
        DecodedReq.erase(0, pos + 1);
    }
    SendEmailInfoList.push_back(DecodedReq);


    string token, receiver, message, subject, External_reciever;
    string::size_type strlen = 0;

    int idx = 0;
    vector<string> AllReceiverList;
    vector<string> ExternalList;

    vector<string> InternalList;
    // --- receviers
    token = "mailTo=";
    pos = SendEmailInfoList[idx].find(token);

    // load receivers full email address to AllReceiverList
    while(pos != string::npos){
        pos += token.size();
        External_reciever = SendEmailInfoList[idx].substr(pos);
        AllReceiverList.push_back(External_reciever);

        idx++;
        pos = SendEmailInfoList[idx].find(token);
    }

    token = "@penncloud.com";
    for(auto name : AllReceiverList){
        pos = name.find(token);
        if(pos != string::npos){
            // if the receiver has a @localhost mail
            // add his username to InternalList
            InternalList.push_back(name.substr(0, pos));
        } else {
            // else add to ExternalList
            ExternalList.push_back(name);
        }
    }

    // --- subject
    token ="title=";
    pos = SendEmailInfoList[idx].find(token);
    pos += token.length();
    subject = SendEmailInfoList[idx].substr(pos);
    idx++;

    // --- message
    token ="content=";
    pos = SendEmailInfoList[idx].find(token);
    pos += token.length();
    message = SendEmailInfoList[idx].substr(pos);

    // generate uid
    string uid;
    string emaildata = date + usrname + subject + message;
    unsigned char digestbuf[2*MD5_DIGEST_LENGTH+1];
    computeDigest(emaildata.c_str(), emaildata.length(), digestbuf);
    char BUFF[32];
    char MD5_id[2*MD5_DIGEST_LENGTH+1];
    memset(MD5_id, 0, sizeof(MD5_id));
    for (int j=0; j < MD5_DIGEST_LENGTH; j++){
        sprintf(BUFF, "%02x", digestbuf[j]);
        strcat(MD5_id, BUFF);
    }

    uid = string(MD5_id);
    //-----------
    for(auto name : ExternalList){
        EmailDTO email2;
        email2.set_sender(usrname);
        email2.set_receiver(name);
        email2.set_message(message);
        email2.set_subject(subject);
        email2.set_date(date);
        email2.set_uid(uid);

        cerr<< "gets here!"<<endl;
//    size_t found1 = email2.receiver().find("@localhost");
//        printf("%s####################%d###########\n",email2.receiver().c_str(),int(found1));
        if (!External_SMTP(email2)) {
            return renderPage("failed.html");
        }
    }

    for(auto name : InternalList){
        cerr<< "gets here inter!"<<endl;
        cerr<< name <<endl;
        cerr<< message <<endl;
        cerr<< subject <<endl;
        cerr<< date <<endl;
        cerr<< uid <<endl;
        EmailDTO email ;
        email.set_sender(usrname);
        email.set_receiver(name);
        email.set_message(message);
        email.set_subject(subject);
        email.set_date(date);
        email.set_uid(uid);
        AddressDTO address = masterServer->RequestRead(name, "password");

        if(address.address() == ""){
            return renderPage("failed.html");
        }

        WriteResponse response = masterServer->RequestWrite(name, uid, email.SerializeAsString(), WriteType::PUT, FileType::EMAIL);
        if(!response.success()){
            return renderPage("failed.html");
        }
    }

    return renderPage("success.html");
}


void HttpService::ParseUAPOST(string &username, string &password){
    // UAPOST stands for useraccount POST
    string POSTreq = Rheader->post_req;
    // fetch the username and password
    string usre = "username=";
    string pwe = "password=";
    string::size_type posend = POSTreq.find('&');

    // parse username
    string::size_type pos = POSTreq.find(usre);
    pos +=(usre).length();
    username = POSTreq.substr(pos,posend - pos);

    // parse password
    pos = POSTreq.find(pwe);
    pos +=(pwe).length();
    password = POSTreq.substr(pos);
    return;
}

int HttpService::HandleRegister() {
    string username, password;
    // Parse username and password from POST req
    ParseUAPOST(username, password);
    // create a UserDTO
    //TODO: Add nickname
    UserDTO user;
    user.set_username(username);
    user.set_password(password);

    if(CreateUser(user)){
        // log the user in right after he/she sign up
        SetCookie(username);
        is_login = true;
        usrname = username;
        return renderPage("success.html");
    } else {
        // sign up failed (user already exist)
        //TODO: maybe we can send some message to indicate the failure signup later.
        return renderPage("failed.html");

    }
}

int HttpService::HandleLogin(){
    string username, password;
    // Parse username and password from POST req
    ParseUAPOST(username, password);
    // create a UserDTO
    UserDTO user;
    user.set_username(username);
    user.set_password(password);

    if(ValidateUser(user)){
        // send cookie to the browser
        SetCookie(username);
        is_login = true;
        usrname = username;
        return renderPage("success.html");
    }else {
        return renderPage("failed.html");
    }
}

void HttpService::RemoveCookie(){
    Rheader->cookie.SessionID = usrname + "=deleted";
    Rheader->cookie.Expire = "expires=Thu, 01 Jan 1970 00:00:00 GMT";
    set_cookie = true;
    is_login =false;
}

int HttpService::SetCookie(const string &username){
    this->set_cookie = true;
    string val = gen_rand_string(17);
    Rheader->cookie.SessionID = username + "=" + val;
    return 0;
}


int HttpService::HandleERR(){
    string Res = "HTTP/1.1 " + Notfound + "\r\n";
    Res += "404 Not Found!";
    if(this->db) cerr<<Res<<endl;

    auto len = Res.length();
    const char * msgbuf = Res.c_str();
    if(db) fprintf(stderr,"[%d] S: %s\n", connfd, msgbuf);
    send(connfd, msgbuf, len, 0);
    return 0;
}


string HttpService::gen_rand_string(size_t length){
    auto randc = []() ->char{
        const char charset[] = "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
        const size_t m_index = (sizeof(charset) - 1);
        return charset[ rand() % m_index];
    };
    string result(length, 0);
    generate_n(result.begin(), length, randc);
    return result;
}


void HttpService::urlDecode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}
/*------------------------
 * Send email externally
 */
bool External_SMTP(EmailDTO email_to_send){
    struct connection conn1;
    // initializeBuffer // HW2: test/common.cc
    conn1.fd = -1;
    conn1.bufferSizeBytes = BufferSizeBytes;
    conn1.bytesInBuffer = 0;
    conn1.buf = (char*)malloc(BufferSizeBytes);
    if (!conn1.buf) panic("Cannot allocate %d bytes for buffer", BufferSizeBytes);
    char HostName[100];
    {
        size_t found = email_to_send.receiver().find('@');
        if (string::npos != found){strcpy(HostName, email_to_send.receiver().c_str() + found+1);}
    }

    // connectToPort // HW: test/common.cc
    bool UPENN_flag = false, QQ_flag = false,Outlook_flag = false, Gmail_flag = false, Hotmail_flag = false;
    int ret_connect = connectToPort(&conn1, HostName, UPENN_flag, QQ_flag, Outlook_flag, Gmail_flag, Hotmail_flag);
    if (0!= ret_connect) {printf("connectToPort() failure\n"); return false;}
    if (UPENN_flag) {
        conn1.bytesInBuffer = 0;
        expectToRead(&conn1,"220 *");
        // ------------------------
        writeString(&conn1, "HELO\r\n"); expectToRead(&conn1,"250 *"); 
        char temp[MAX_LEN_CMD];
        sprintf(temp,"HELO %s\r\n",HostName);
        writeString(&conn1, temp); expectToRead(&conn1,"250 *"); 
        // ------------------------
        size_t found1 = email_to_send.sender().find("@");
        size_t found2= email_to_send.sender().find(".");
        if ( string::npos !=found1 and string::npos!=found2 ) 
            sprintf(temp, "MAIL FROM:<%s>\r\n", email_to_send.sender().c_str());
        else {
            char SENDER[MAX_LEN_CMD]; strcpy(SENDER,email_to_send.sender().c_str() );
            char *NAME = strtok(SENDER,"@");
            sprintf(temp, "MAIL FROM:<%s@penncloud.com>\r\n", NAME);
        }
        writeString(&conn1, temp);  
        if (!expectToRead(&conn1,"250 *")){printf("MAIL FROM: followed by error command!!\n"); return false;}
        if (Outlook_flag){expectToRead(&conn1,"250 *");}
        // ------------------------
        sprintf(temp, "RCPT TO:<%s>\r\n", email_to_send.receiver().c_str());
        writeString(&conn1, temp); 
        if (!expectToRead(&conn1,"250 *")) {printf("RCPT TO: followed by error command!!\n"); return false;}
        // ------------------------
        writeString(&conn1, "DATA\r\n"); 
        if (!expectToRead(&conn1,"354 *")) {printf("DATA: followed by error command!!\n"); return false;}
        // ------------------------
        if ( string::npos !=found1 and string::npos!=found2 ) 
            sprintf(temp, "FROM: %s\r\n", email_to_send.sender().c_str());
        else {
            char SENDER[MAX_LEN_CMD]; strcpy(SENDER,email_to_send.sender().c_str() );
            char *NAME = strtok(SENDER,"@");
            sprintf(temp, "FROM: %s@penncloud.com\r\n", NAME);
        }
        writeString(&conn1, temp); 
        // ------------------------
        sprintf(temp, "TO: %s\r\n", email_to_send.receiver().c_str());
        writeString(&conn1, temp);
        // ------------------------
        sprintf(temp, "Subject: %s\r\n", email_to_send.subject().c_str());
        writeString(&conn1, temp);
        // ------------------------
        sprintf(temp, "Date: %s\r\n", email_to_send.date().c_str());
        writeString(&conn1, temp);
        // ------------------------
        sprintf(temp, "%s\r\n", email_to_send.message().c_str());
        writeString(&conn1, temp);
        writeString(&conn1, ".\r\n"); expectToRead(&conn1,"");
        writeString(&conn1, "QUIT");
        close(conn1.fd);
        // ------------------------
        return  true;
    } else {                                                
        char Arbitrary[]="Arbitrary error command\r\n";
        conn1.bytesInBuffer = 0;
        expectToRead(&conn1,"220 *");
        char temp[MAX_LEN_CMD]; sprintf(temp, "EHLO %s\r\n",HostName);
        // ------------------------
        writeString(&conn1,temp); 
        int num_loop =1;
        if (Hotmail_flag) num_loop=8; 
        if (QQ_flag) num_loop =3;
        if (Outlook_flag) num_loop = 8; 
        if (Gmail_flag) num_loop = 7;
        for (int i=0; i < num_loop;i++) expectToRead(&conn1,"250-*"); 
        // sleep(5);
        if (QQ_flag or Gmail_flag){expectToRead(&conn1,"250 *");
            writeString(&conn1, Arbitrary); expectToRead(&conn1,"502 *"); 
        }else {
            writeString(&conn1, Arbitrary); expectToRead(&conn1,"500 *"); 
            expectToRead(&conn1,"250 *");
        }
        // expectToRead(&conn1,"test");
        // ------------------------
        size_t found1 = email_to_send.sender().find("@");
        size_t found2= email_to_send.sender().find(".");
        if ( string::npos !=found1 and string::npos!=found2 ) 
            sprintf(temp, "MAIL FROM:<%s>\r\n", email_to_send.sender().c_str());
        else {
            char SENDER[MAX_LEN_CMD]; strcpy(SENDER,email_to_send.sender().c_str() );
            char *NAME = strtok(SENDER,"@");
            sprintf(temp, "MAIL FROM:<%s@penncloud.com>\r\n", NAME);
        }
        writeString(&conn1, temp);  
        if (!expectToRead(&conn1,"250 *")){printf("MAIL FROM: followed by error command!!\n"); return false;}
        // ------------------------
        writeString(&conn1, Arbitrary); if (QQ_flag)expectToRead(&conn1,"502 *");  else expectToRead(&conn1,"500 *");
        sprintf(temp, "RCPT TO:<%s>\r\n", email_to_send.receiver().c_str());
        writeString(&conn1, temp); 
        if (!expectToRead(&conn1,"250 *")) {printf("RCPT TO: followed by error command!!\n"); return false;}
        // ------------------------
        writeString(&conn1, Arbitrary); if (QQ_flag)expectToRead(&conn1,"502 *");  else expectToRead(&conn1,"500 *");
        writeString(&conn1, "DATA\r\n"); 
        if (!expectToRead(&conn1,"354 *")) {printf("DATA: followed by error command!!\n"); return false;}
        // if (QQ_flag) 
        //     if (!expectToRead(&conn1,"550 *")) {printf("after DATA 550 QQ: followed by error command!!\n"); return false;}
        // else {
        //     if (!expectToRead(&conn1,"250 *")) {printf("after DATA 250 outlook/hotmail: followed by error command!!\n"); return false;}
        // }
        // ------------------------
        if ( string::npos !=found1 and string::npos!=found2 ) 
            sprintf(temp, "FROM:<%s>\r\n", email_to_send.sender().c_str());
        else {
            char SENDER[MAX_LEN_CMD]; strcpy(SENDER,email_to_send.sender().c_str() );
            char *NAME = strtok(SENDER,"@");
            sprintf(temp, "FROM:<%s@penncloud.com>\r\n", NAME);
        }
        writeString(&conn1, temp); 
        // ------------------------
        sprintf(temp, "TO:<%s>\r\n", email_to_send.receiver().c_str());
        writeString(&conn1, temp);
        // ------------------------
        sprintf(temp, "Subject: %s\r\n", email_to_send.subject().c_str());
        writeString(&conn1, temp);
        // ------------------------
        sprintf(temp, "Date: %s\r\n", email_to_send.date().c_str());
        writeString(&conn1, temp);
        // if (!QQ_flag and !Gmail_flag){sprintf(temp, "Message-ID: <%s>\r\n", email_to_send.uid().c_str()); // stil does not work
        //     writeString(&conn1,temp);
        //     sprintf(temp,"boundary=\"------------%s\"\r\n",email_to_send.uid().c_str());
        //     writeString(&conn1,temp);
        //     writeString(&conn1,"Content-Language: en-US\r\n");
        //     sprintf(temp,"------------%s\r\n",email_to_send.uid().c_str());
        //     writeString(&conn1,temp);
        //     writeString(&conn1,"Content-Type: text/plain; charset=utf-8;8; format=flowed\r\nContent-Transfer-Encoding: 7bit\r\n");
            
        // }
        // ------------------------
        sprintf(temp, "%s\r\n", email_to_send.message().c_str());
        writeString(&conn1, temp);
        // if (!QQ_flag and !Gmail_flag){sprintf(temp,"------------%s\r\n",email_to_send.uid().c_str());
        //     writeString(&conn1,temp);}
        writeString(&conn1, ".\r\n"); expectToRead(&conn1,"");
        writeString(&conn1, "QUIT");
        close(conn1.fd);
        // ------------------------
        return  true;
    }
}

