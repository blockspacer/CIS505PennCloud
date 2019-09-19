readme.txt

-----Below is Yuding talking---------------------

feserver

compile: make
run: ./feserver -v
where the -v is the debug flag

defualt port number is 8888

URL:
localhost:8888/home
localhost:8888/inbox
localhost:8888/drive
localhost:8888/login

The current stucture of FEserver looks like the following:

FEserver shoule have 3 services: 
    -HttpService (We only have this one for now)
    -WebMailService
    -Storage

I am still not so sure about what to do with useraccount. For now, my thought is to just 
take it as an instance variable in FEserver. And when we do a POST request, Httpservice
will feed the info into useraccount.

When FEServer gets Some request : 
    1. FEserver will create a worker thread
    2. The worker thread will Parse the request and decide which service to use
    3. For now since the only service we have is HttpService, so it just
        create a HttpService instance to handle the http request
    4. Once the request gets handled, the thread exists and the 
        HttpService instance gets destroyed.

---Nov.12 Yuding------------------------------------
I also added a temporary mailbox for testing without backend Nov. 12

---Nov.15 Yuding------------------------------------

Added a very simple login and logout mechanism, which basically does the following:
    1. When user login in. the server check the username and password
        (for now the only valid user is username:cis505 password:penncloud)
    2. if the username and password match, then hand cookie to browser,
        with this cookie, the next time the browser send Req, the server
        will treat is as a login user with username as key (say cis505)
        (for now the Cookie is just "username=<random17char string>"
    3. When user click logout, the server clears the Cookie from server side

Here is what happens when go to our Penncloud webside:
    - When User get into the homepage, if the Browser Request do not include Cookie
      Then the only page the user can go is /home and /login

    - Once the user login, (using username:cis505 password:penncloud), Then WebMail
        and Storage page will be available and the server will know the username from Cookie.
        Using the username as key, the server can then communicate with backend side and process
        the proper Webmail page and storage page and send it back to Browser

---Nov.16 Yuding------------------------------------
Did the following:
A. Using gRPC to extract Emails from the Backend side and pass those message to the WebMail Page
    1. Add EmailRepositoryClient *EC; UserRepositoryClient *UC and StorageRepositoryClient *SC as private variable in
        HttpService obj;
    2. Create the concrete instances of EmailRepositoryClient, UserRepositoryClient and StorageRepositoryClient
        client server at the main function (in feserver.cc). Then pass their address as argument to HttpService
        constructor
    3. In httpservice.cc: I extract emails from the backend server (the sample test server at method:
            int AddMailul(stringstream &html_content); // for experiment only
       So from where, I used the gRpc  getAllEmails("user") call to extract all emails from Backend
       Then feed those Emails into the output html feedback to the browser.

B. Revised the httpserver to support javascript and css. add some very basic javascript and css features into the
    /inbox page as to prepare for later usage
    (such as showing the email content when user click on the message, remove message etc.)


---Nov. 18 Yuding ----------------------------------

    1. Proposed the following routes:
        For a login user:  the user_id  here is equivalent to username for now
            homepage: /home/user_id    or  /home/cookie_val corresponding to the User_id
            inbox: /inbox/user_id
            open one email: /inbox/user_id/mail_uid
            compose a new email: /inbox/user_id/compose
            storage: /drive/user_id

    2. A Note about DELETE method:
    /*
     *  I learned from an online course that html do not support
     *  DELETE form directly so for the DELETE method, we hack one
     *  using POST method: something like
     *
     *  <form action="/inbox/user_id/mail_id?_method=DELETE" method="POST">
     *       <button class="btn btn-danger">Delete</button>
     *  </form>
     *
     *  And from where, we could parse the info from POST req
     *      - _method=DELETE (so we know we supposed to delete the email from database)
     *      - And by parsing the URL, we got the the key to access this email
     *          would be the tuple (user_id,mail_id )
     *      - From where, we call WebMailService.DELETE(username, email_uid)
     *          to delete the email from database
     */