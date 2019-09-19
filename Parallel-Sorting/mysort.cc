//cp -rf /home/cis505/git/  /media/sf_sf_CIS505/
// cp -rf /media/sf_sf_CIS505/git/ /home/cis505/
#include<iostream>
#include<fstream>
#include<vector>  // suggested by PIAZZA
using namespace std;
// processes  -------- refers to lecture note page 16
#include <sys/types.h>
#include <sys/wait.h>// stack overflow -- https://www.google.com/search?source=hp&q=wait+was+not+declared+in+this+scope&oq=wait+was+not+declared+in+this+scope&gs_l=psy-ab.3..0.561.8467.0.8742.45.35.4.0.0.0.298.4292.0j21j4.25.0....0...1.1.64.psy-ab..16.28.4171...0i131k1j0i10k1j0i3k1j0i22i30k1j0i22i10i30k1j0i13k1j0i13i30k1.0.LED1vznnoJE
#include <unistd.h>
#include <stdio.h>
// processes  -------- refers to lecture note page 16
#include<stdlib.h>
#include<errno.h>
#include<string.h>
// threads
#include<pthread.h>
// shared memory --  https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html#SECTION002731000000000000000
#include <sys/ipc.h>
#include <sys/shm.h>
#include <algorithm> // DEBUG



int Sort_via_processes(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr);
int Sort_via_processes_shared_memo(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr);
int Sort_via_thread(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr);
int Merge_sorted_groups(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr);

int main(int argc, char *argv[])
{
//    // TEST -- https://stackoverflow.com/questions/259297/how-do-you-copy-the-contents-of-an-array-to-a-stdvector-in-c-without-looping
//    long long ar_test[4] = {1,3,5,7};
//    unsigned datasize = sizeof(ar_test) / sizeof(long long);
//    vector <long long> V_test;
//    V_test.resize(datasize);
//    memcpy(&V_test[0], &ar_test[0], datasize *sizeof(long long) );
//    for (int i = 0; i<4; i++) cout << V_test[i] << '\t' << ar_test[i] << '\n';
//    return 0;
    
    int num_process = 4;
    //cout<< getcwd(NULL,0);
    char ch; // control symbol
    bool thread_flag = false, lexicographic_flag = false, shared_memo_flag = false;
    while (( ch = getopt( argc, argv, "n:tLs")) != -1) // refers to https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
        // "nt:" does not work -- n need a following argument, while t does not
        switch (ch) {
            case 'n':
                num_process = atoi(optarg);
                if (!(int( num_process ) == num_process && num_process >0) )
                {cout<<"Wrong number of process:"<<num_process<<'\n'; exit(1);}
                break;
            case 's':
                shared_memo_flag = true; break;
            case 't':
            	thread_flag = true; break;
            case 'L':
                lexicographic_flag = true; break;
                /*		default:
                 cerr<< "abort (incorrect input format)"; abort();*/
        }
    if (shared_memo_flag) thread_flag = false;
    if (!lexicographic_flag)
    {
        vector <long long> arr;
        /* --------------------------------------
         * read files
         * --------------------------------------*/
        long long num_temp = 0;
        //cout << argv[optind] <<'\n'; // DEBUG
        for (int i = optind; i <argc;  i++)
        {
         //   cout<<"i:"<<i<<" argc:"<< argc <<"\n";
            ifstream infile(argv[i]);
            if (infile.is_open()) // cannot use infile.open(argv[i])
                while (infile >> num_temp ) arr.push_back(num_temp);
            else
            {cerr<<"error opening: " << argv[i] <<'\n'; exit(1);}
            infile.close();
        }
        //cout << "arr.size:" << arr.size() << '\n';
        vector <long long > sorted_arr;
        long long size_avg = arr.size() /num_process;
        if ( (1 == num_process) || (size_avg <2)){//handout N=1
            int BubbleSort(vector <long long> &arr); // DECLARATION
            BubbleSort(arr);
            for(long long j =0; j < arr.size(); j++) sorted_arr.push_back(arr[j]);
        }
        else
        {
            // otherwise, first split data into num_process groups (vector class again)
            vector <vector <long long> > groups;
           //cout << "\nsize avg:" << size_avg<< "\n";
            for (int i = 0 ; i <num_process - 1; i++){
                vector <long long> v; // clean up v by redefining it
                for (long long j = 0; j < size_avg; j++) { v.push_back(arr[j+size_avg *i]); }
                groups.push_back(v);
           //     cout <<"\n" <<groups[i].size() << "\n";

            }
            {
                vector <long long> v;
                for (long long j = size_avg*(num_process-1); j < arr.size(); j++)  v.push_back(arr[j]);
                groups.push_back(v);
            }
    //        cout <<"correctly grouping numbers.\n";
            if (thread_flag)
            {
                int Sort_via_thread(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr);
                Sort_via_thread(num_process,groups, sorted_arr);
            }
            else if (shared_memo_flag)
            {
                int Sort_via_processes_shared_memo(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr);
                Sort_via_processes_shared_memo(num_process,groups, sorted_arr);
            }
            else {
                int Sort_via_processes(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr);
                Sort_via_processes(num_process,groups, sorted_arr);
            }
            groups.clear();
        }
        arr.clear();
//        // output
//       long long  counter     = 0;
//        cout << "sorted_arr size:\n" << sorted_arr.size() << '\n';
//
//        for (long long i =0; i < sorted_arr.size() - 1; i++)
//            if (sorted_arr[i] <= sorted_arr[i+1])
//            {counter ++; // cout << "i:"<<i<<" "<<sorted_arr[i]<<'\t' << sorted_arr[i+1] <<'\n';
//            }// DEBUG (surprising that two platforms provides different answer!!!)
//       cout <<"counter = "<<counter << '\n'; // I do not know why?
        for (long long i =0; i < sorted_arr.size(); i++)
            cout <<  sorted_arr[i]<<'\n';
    
        sorted_arr.clear();
    }else{
        vector <string> arr;
        /* --------------------------------------
         * read files
         * --------------------------------------*/
        string str_temp;
        //cout << argv[optind] <<'\n'; // DEBUG
        for (int i = optind; i <argc;  i++)
        {
            //   cout<<"i:"<<i<<" argc:"<< argc <<"\n";
            ifstream infile(argv[i]);
            if (infile.is_open()) // cannot use infile.open(argv[i])
                while (infile >> str_temp ) arr.push_back(str_temp);
            else
            {cerr<<"error opening: " << argv[i] <<'\n'; exit(1);}
            infile.close();
        }
        //cout << "arr.size:" << arr.size() << '\n';
        vector <string> sorted_arr;
        long long size_avg = arr.size() /num_process;
        if ( (1 == num_process) || (size_avg <2)){//handout N=1
            int BubbleSort_S(vector <string> &arr); // DECLARATION
            BubbleSort_S(arr);
            for(long long j =0; j < arr.size(); j++) {sorted_arr.push_back(arr[j]);
                cout << sorted_arr[j] << '\n';
            }
        }
        else
        {cout << "I do not think there are anything substantial converting to string issues \n-- therefore I do not convert parallel part for strings\n";
        }
        arr.clear();
        sorted_arr.clear();
    }
    return 0;
}




int Sort_via_processes(int num_process, vector<vector <long long > > &groups,  vector <long long > &sorted_arr)
{   /*---------------------------
	 * Sort use processes with PIC via pipe
	 */
    
    /* --------------
     * fork num_process of processes -- must be put at the beginning separately
     */
    vector <pid_t> pids; // used to create segment fault
    for (int i = 0; i< num_process;i++) pids.push_back(0); // initialization
    int up_pipe[num_process][2], down_pipe[num_process][2];
    int pid = 0; // receive signal after the fork
    int pipe_id; // variables used by each child process after the fork
    for (pipe_id = 0; pipe_id < num_process; pipe_id++){
        // handout+PIAZZA: "The handout asks you to use a pair of pipes for each subprocesses. So please follow the handout and use an up pipe and a down pipe"
        pipe(up_pipe[pipe_id]); pipe(down_pipe[pipe_id]); // write end + read end -- slide 2 page 26 -- before fork()
        // fork a child process -- lecture slide 2 page 16
        pid  = fork();
//        kill(0, SIGSTOP); // in order to be able to attach to its child processes
        // https://stackoverflow.com/questions/20161144/command-line-application-how-to-attach-a-child-process-to-xcode-debugger
        if ( -1 == pid ) {perror("fork error");exit(1);}
        else if (0 == pid) { // a child process/ subprocess
            // write end of the down pipe and the read end of the up pipe
            // are not used by child
            close(down_pipe[pipe_id][1]);close(up_pipe[pipe_id ][0]);
            break;
           } else { // parent process
            pids[pipe_id] = pid;
            // read end of the down pipe and the write end of the up pipe
            // are not used by the parent
            close(down_pipe[pipe_id][0]);close(up_pipe[pipe_id][1]);
        }
    }
    /*--------
     * child processes receive data and sort while parent process write data via pipe
     -------------*/
    vector <long long > own_vector;// variables used by each child process after the fork
    if (0 == pid) { // a child process/ subprocess
        // read numbers assigned by parent process into its own vector
        FILE *child_receive_from_parent_down_pipe = fdopen(down_pipe[pipe_id][0], "rb");
        if (NULL == child_receive_from_parent_down_pipe) {cerr << "child_receive_from_parent_down_pipe:"<< strerror(errno);}
        else {
            char buffer_num[27]; //http://people.cs.uchicago.edu/~dmfranklin/tutorials/fgets.txt
            for(int i =0; i < groups[pipe_id].size(); i++)
            {   fgets(buffer_num,25, child_receive_from_parent_down_pipe) ;
                own_vector.push_back( stoll(buffer_num) ); // atoi does not work
            }
            
            fclose(child_receive_from_parent_down_pipe);
            close(down_pipe[pipe_id][0]);
            if ( own_vector.size() >0) {
            	int BubbleSort(vector <long long> &arr);
            	BubbleSort(own_vector);
            }
        }
    } else { // parents process
        // it should write the numbers that it wants that subprocess to sort to the down pipe --handout
        for(int i = 0; i < num_process; i++ )
        {
            FILE *parent_sent_to_child_via_down_pipe = fdopen(down_pipe[i][1],"wb");// FILE object -- handout
            if (NULL == parent_sent_to_child_via_down_pipe) {cerr << "parent_sent_to_child_via_down_pipe"  <<  strerror(errno);}
            // write number in group i into FILE
            //cout << groups[i].size() << '\n'; // DEBUG
            for (long long j = 0; j < groups[i].size(); j++) {
                fprintf(parent_sent_to_child_via_down_pipe,"%lld\n",groups[i][j]);
            }
            
            fclose(parent_sent_to_child_via_down_pipe);
            close(down_pipe[i][1]);
        }
    }
    
    /*--------
     * child processes write data to the pipe while parent receives data
     -------------*/
    
    if (0 == pid) { // a child process/ subprocess 
        FILE * child_feedback_sent_to_parent_via_up_pipe = fdopen(up_pipe[pipe_id][1],"wb");
        if (NULL == child_feedback_sent_to_parent_via_up_pipe) {cerr << "child_feedback_sent_to_parent_via_up_pipe" <<strerror(errno);}
        for(long long i = 0; i< own_vector.size(); i++)
            fprintf(child_feedback_sent_to_parent_via_up_pipe, "%lld\n", own_vector[i]);
        own_vector.clear();
        fclose(child_feedback_sent_to_parent_via_up_pipe);
        exit(0);//return 0;
    } else { // parents process
        // Below does not seem to have any improvement
        // vector <int> child_finished;
        // for (int i = 0; i < num_process; i++) child_finished.push_back(false);

        int child_pending = num_process, child_pid;
        for (int i = 0; i < num_process; i++)
        {
            FILE * parent_receive = fdopen(up_pipe[i][0], "rb");
            if (NULL == parent_receive) {cerr <<"parent receive2 " << strerror(errno);exit(1);}
            else {
                char buffer_num[100];
                for (long long j = 0 ; j < groups[i].size(); j ++)
                { fgets(buffer_num,100,parent_receive);
             //       cout << "buffer_num<<<<<<<<" << buffer_num << ">>>>>>.\n";
                    //    cout << "groups[Pi_id][j]" << groups[i][j] << '\n';
                    groups[i][j] = stoll(buffer_num);
                    child_pending --;
                }
                fclose(parent_receive);
                // close(up_pipe[i][0]);
            }
        }
//        while (child_pending >0)
//            if ( -1  ==  (child_pid =waitpid(-1,NULL,WNOHANG)) ) {cerr << strerror(errno); exit(1);}
//            else if ( 0  == (child_pid = waitpid(-1,NULL,WNOHANG) ) ) // no state of child process has been changed -- potential deadlock
//            {
//
//            }
//            else {
//                int Pi_id;
//                for ( Pi_id = 0; Pi_id < num_process; Pi_id++)
//                    if (child_pid == pids[Pi_id] ) break;
//                if (not(child_pid == pids[Pi_id] ) ) {cerr <<"error storing child processes' PID!"; exit(1);}
//                FILE * parent_receive = fdopen(up_pipe[Pi_id][0], "rb");
//                if (NULL == parent_receive) {cerr <<"parent receive" << strerror(errno);}
//                char buffer_num[100];
//                for (long long j = 0 ; j < groups[Pi_id].size(); j ++)
//                { fgets(buffer_num,25,parent_receive);
//                 //   cout << "buffer_num" << buffer_num << '\n';
//                 //  cout << "groups[Pi_id][j]" << groups[Pi_id][j] << '\n';
//                    groups[Pi_id][j] = stoll(buffer_num);
//                }
//                fclose(parent_receive);
//               // close(up_pipe[Pi_id][0]);
//                child_pending --;
////                if (103 > child_pending)
////                {
////                    cout << Pi_id << '\t';
////                }
//            }
        for (int i = 0; i < num_process; i++)
        { close(up_pipe[i][0]); close(up_pipe[i][1]);}
      
        Merge_sorted_groups(num_process, groups, sorted_arr);
        return 0;
     }
    // return 0;
}




int Sort_via_thread(int num_process, vector<vector <long long > > &groups,  vector <long long > &sorted_arr)
{	// slide 2 - page  46
	void *BubbleSort_thread(void *pt_arg);
    int up_pipe[num_process][2], down_pipe[num_process][2];
	pthread_t thread_id[num_process];
    int pipe_id;
	for ( pipe_id = 0; pipe_id < num_process; pipe_id++) {
        pipe(up_pipe[pipe_id]); pipe(down_pipe[pipe_id]);
		if (groups[pipe_id].size() > 0) {
			pthread_create(&thread_id[pipe_id], NULL, BubbleSort_thread, &groups[pipe_id]);
            if( 0 == pthread_self() ){break;}// do not close some ends at the beginning @76 PIAZZA
            // the function as the thread callback must be a (void*)(*)(void*) type function
		}
	}

    vector <long long> own_vector;// vector used by each thread
    if (0 == pthread_self())
    {
        //cout << "Within a child thread\n"; // this is not done at all
        FILE *child_receive_from_parent_down_pipe = fdopen(down_pipe[pipe_id][0], "rb");
        if (NULL == child_receive_from_parent_down_pipe) {cerr << "child_receive_from_parent_down_pipe:" << strerror(errno);exit(1);}
        else
        {
            char buffer_num[27];
            for(int i = 0; i < groups[pipe_id].size(); i++)
            {
                fgets(buffer_num, 25, child_receive_from_parent_down_pipe);
                own_vector.push_back( stoll (buffer_num));
            }
            fclose(child_receive_from_parent_down_pipe);
            
        }
        /// Bubble has already been done?
    }
    
	void *status; // what is the use of this
	for (int i=0; i<num_process ; i++)
		pthread_join(thread_id[i], &status);
    for(int pipe_id =0; pipe_id <num_process; pipe_id ++)
    {
        close(up_pipe[ pipe_id][0] ); close(up_pipe[ pipe_id][1] );
        close(down_pipe[ pipe_id][0] ); close(down_pipe[ pipe_id][0] );
    }
	Merge_sorted_groups(num_process, groups, sorted_arr);
	return 0;
}
int Merge_sorted_groups(int num_process, vector<vector <long long > > &groups,  vector <long long >& sorted_arr)
{
    // http://www.geeksforgeeks.org/merge-two-sorted-arrays/
    vector <long long> arr1;
     arr1 = groups[0];
    for (int g_id = 1; g_id < num_process; g_id ++)
    {
        long long ind_first = 0, ind_sec = 0;
        vector <long long> merged_temp_arr;
        while (ind_first < arr1.size() && ind_sec < groups[g_id].size())
        {
           if (arr1[ind_first] < groups[g_id][ind_sec])
            {merged_temp_arr.push_back( arr1[ind_first]);
            ind_first ++;
            }
            else
            {   merged_temp_arr.push_back( groups[g_id][ind_sec]);
              ind_sec ++;
            }
        }
         while (ind_first < arr1.size() )
        {
           merged_temp_arr.push_back( arr1[ind_first]);
            ind_first ++;
        }
         while (ind_sec < groups[g_id].size() )
        {
           merged_temp_arr.push_back( groups[g_id][ind_sec]);
            ind_sec ++;
        }
        arr1 = merged_temp_arr;
    }
    sorted_arr = arr1;
    return 0;
}
//
#define SIZE_LL (sizeof(long long))// Cannot allocate memory
//#define SHMSZ     (10000*SIZE_LL)
//#define SHMSZ





int Sort_via_processes_shared_memo(int num_process, vector<vector <long long > > &groups, vector <long long >&sorted_arr)
{
    // DECLARATION
    //int OPEN_shm_sign(int *shm_sign_pt,long long num_process);
    key_t key =5678, key_signal_wait = 911; // I only create one shared array
    /*---------------------------
     * Sort use processes with PIC via shared memeory -- https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html#SECTION002731000000000000000
     */
    /* --------------
     * fork num_process of processes -- must be put at the beginning separately
     */
    vector <pid_t> pids; // used to create segment fault
    for (int i = 0; i< num_process;i++) pids.push_back(0); // initialization
    int pid = 0; // receive signal after the fork
    
    long long SHMSZ = 0;
    //vector <bool> finish_input,finish_update;
    for (int i = 0; i < num_process; i++)
        {
            SHMSZ = SHMSZ+ groups[i].size();
        }
    
    
    long long SHMID, *SHM_SIGN_PT;
    if ((SHMID = shmget( key, ((2*num_process+SHMSZ) * SIZE_LL), IPC_CREAT | 0666 ) ) < 0) //IPC_CREAT | 0644 // IPC_CREAT represents creating procedure
        // first 2*num_process are SIGNALS of waits
    	{cerr << "when creating segments:" << strerror(errno)<< '\n';exit(1);}
    
    if ((SHM_SIGN_PT = (long long *)(shmat(SHMID, NULL, 0))) == (long long *) -1){perror("shmat OPEN shm sign");exit(1);}
    
    int child_id;
    for (child_id = 0; child_id < num_process; child_id ++){
        // fork a child process -- lecture slide 2 page 16
        if ( -1 == (pid = fork()) ) {perror("fork error");exit(1);}
        else if (0 == pid) {break;}
        SHM_SIGN_PT[child_id*2] = 0;SHM_SIGN_PT[child_id*2+1] = 0;
    }
    shmdt( (void *) SHM_SIGN_PT);

    vector <long long > own_vector;// variables used by each child process after the fork
    if ( 0 == pid )
    {// child process read data and do sorting
        long long shmid, *shm;// temporary address +  temporary: parent process uses them to create the segment
        if ((shmid = shmget(key, (SHMSZ * SIZE_LL), 0666) ) < 0){perror("shmget child process read data and do sorting");exit(1);}
        if ((shm = (long long *)(shmat(shmid, NULL, 0))) == (long long *) -1) {perror("shmat");exit(1);}

        while ( 0 == shm[child_id*2]) sleep(1);
        long long shift= 2*num_process;
        if (child_id >0)
            for (int i = 0; i < child_id; i++) shift += groups[i].size(); // not child_id -1
        for (long long i = 0;  i < groups[child_id].size(); i++)
            own_vector.push_back( shm[shift + i] );
        
        own_vector.resize(groups[child_id].size());
        memcpy(&own_vector[0], &shm[shift], groups[child_id].size()*SIZE_LL);
//        for (int i =0; i < 3; i++) { // DEBUG
//            cout << '\n' << own_vector[i] << '\t' << shm[shift + i]  << '\n';
//        }
        // After DEBUG, groups[child_id] seems to be ordered
        ///  NO ANY PROBLEM
//        long long ctr_int2 = 0;// DEBUG
//        for (long long i= 0; i < groups[child_id].size() - 1; i ++)
//            if (shm[shift+i] != own_vector[i]) {ctr_int2++; }
//        cout << child_id << " ERR "  << ctr_int2 << " start:" << shift << " end:"<< shift + groups[child_id].size()  << '\n'; // DEBUG
//
//        for (long long i= 0; i < groups[child_id].size() - 1; i ++)
//            if (shm[shift+i] != own_vector[i])
//                cout << "index " << shift+i << ":"<< shm[shift+i] << '\t' <<  own_vector[i] << '\n';
// // DEBUG
        shmdt((void *) shm);
        int BubbleSort(vector <long long> &arr);
        BubbleSort(own_vector);
    }else {// parent process puts data into shared memory
        
        long long shmid, *shm;;// temporary address + temporary: parent process uses them to create the segment
 		//intialization of shm as well
        if ((shmid = shmget(key, (SHMSZ * SIZE_LL), 0666) ) < 0){perror("shmget");exit(1);}
        if ((shm = (long long  *)(shmat(shmid, NULL, 0))) == (long long  *) -1){perror("shmat");exit(1);}

        long long  shift = num_process *2;
        for (int child_id = 0; child_id < num_process; child_id ++)
        {
//            for (long long i = 0; i < groups[child_id].size(); i ++)
//                { shm[shift] = groups[child_id][i];shift ++;}
            //finish_input[child_id] = true;
            
            memcpy(&shm[shift], &groups[child_id][0], groups[child_id].size()*SIZE_LL);
            
//     //After DEBUG, groups[child_id] seems to be ordered
//    //  NO ANY PROBLEM
//            long long ctr_int2 = 0;// DEBUG
//            for (long long i= 0; i < groups[child_id].size() - 1; i ++)
//                if (shm[shift+i] != groups[child_id][i]) {ctr_int2++; }
//            cout << child_id << " ERR "  << ctr_int2 << " start:" << shift << " end:"<< shift + groups[child_id].size()  << '\n'; // DEBUG
//
//            for (long long i= 0; i < groups[child_id].size() - 1; i ++)
//                if (shm[shift+i] != groups[child_id][i])
//                    cout << "index " << shift+i << ":"<< shm[shift+i] << '\t' <<  groups[child_id][i] << '\n';
//     // DEBUG
            shift += groups[child_id].size();
//            if (find(own_vector.begin(), own_vector.end(),0) != own_vector.end() )
//                cout << child_id << " here is the error!\n";
            shm[child_id*2] = 1;
        }
        shmdt((void *) shm);
      }
    if ( 0 == pid )
    {    /* Now put sorted own vectors into the memory for the
          * other process to read.
          */

        long long shmid,*shm;// temporary address +  temporary: parent process uses them to create the segment
        if ((shmid = shmget(key, (SHMSZ * SIZE_LL), 0666) ) < 0){perror("shmget");exit(1);}
        if ((shm = (long long *)(shmat(shmid, NULL, 0))) == (long long *) -1){perror("shmat");exit(1);}
        long long  shift = num_process *2;
        if (child_id >0)
            for (int i = 0; i < child_id ; i++) shift += groups[i].size();
//        for (long long i = 0;  i < groups[child_id].size(); i++)
//            shm[shift+i] =own_vector[i];
        memcpy(&shm[shift], &own_vector[0], groups[child_id].size()*SIZE_LL);
//        for (int i =0; i < 3; i++) {
//            cout << '\n' << shm[shift+ i] << '\t' << own_vector[i]  << '\n';
//        }
//        long long ctr_int =0, ctr_int2=0;// DEBUG
//        for (long long i= 0; i < groups[child_id].size()-1; i ++)
//        {
//            if (shm[shift+ i] != own_vector[i] ) ctr_int++;
//            if (shm[shift+ i] < shm[shift+ i+1] ) ctr_int2++;
//        }
//        cout << child_id <<" group size:" << groups[child_id].size() << " ERR " << ctr_int << " " <<ctr_int2 << " start:" << shift << " end:" << shift + groups[child_id].size() -1 << '\n'; // DEBUG
        shm[child_id*2+1] = 1;
        shmdt((void *)shmid);//shmdt((void *)shm_sgn_pt);
        exit(0);
    }else { // parent process
        long long shmid,*shm;// temporary address +  temporary: parent process uses them to create the segment
        if ((shmid = shmget(key, (SHMSZ * SIZE_LL), 0666) ) < 0){perror("shmget");exit(1);}
        if ((shm = (long long *)(shmat(shmid, NULL, 0))) == (long long *) -1){perror("shmat");exit(1);}
        int num_child_pending = num_process;
        while (num_child_pending >0 )
            for (int child_id = 0; child_id < num_process; child_id ++)
            {
                if ( 1 ==shm[child_id*2+1] && num_child_pending >0){
                    shm[child_id*2+1] = 0;
                    num_child_pending--;
                    long long shift= 2 * num_process;
                    if (child_id >0)
                        for (int i = 0; i < child_id; i++) shift += groups[i].size();// not i < child_id -1
                    for (long long i= 0; i < groups[child_id].size(); i ++)
                        groups[child_id][i] = shm[shift+i];
     
                }
            }
        shmdt((void *)shmid);//shmdt((void *)shm_sgn_pt);
        Merge_sorted_groups(num_process, groups, sorted_arr);
        
        shmctl(SHMID, IPC_RMID, NULL); //http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/shm/shmdt.html
        //shmctl(SHM_SIGNAL,IPC_RMID, NULL);
        
    }
    
    return 0;
}




int BubbleSort(vector <long long> &arr)
{
    long long	 n =  arr.size();
    long long temp;
    for (long long i = 0; i < n -1; i++)
        for (long long j = 0; j< n-i-1;  j++)
            if (arr[j] > arr[j+1]) {temp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = temp; }
    return 0;
}

void *BubbleSort_thread(void *pt_arg)
{// conversion of type
    vector <long long>* arr = (vector <long long > *) pt_arg;
    long long n =  arr->size();
    long long temp;
    for (long long i = 0; i < n -1; i++)
        for (long long j = 0; j< n-i-1;  j++)
            if ((*arr)[j] > (*arr)[j+1]) {temp = (*arr)[j]; (*arr)[j] = (*arr)[j+1]; (*arr)[j+1] = temp; }
    return 0;
}


/*---------------------
 * for strings
 *---------------*/


 int BubbleSort_S(vector <string> &arr)
 {
 long long   n =  arr.size();
 string temp;
 for (long long i = 0; i < n -1; i++)
 for (long long j = 0; j< n-i-1;  j++)
 if (arr[j] > arr[j+1]) {temp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = temp; }
 return 0;
 }
