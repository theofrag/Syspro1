#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <string.h>
#include "common.h"
#include "listener.h"
#include <signal.h>

using namespace std;

#define READ 0
#define WRITE 1
#define PERMS 0666

// set queue to global so it can be handled by sa_handler
static queue <pid_t> workersQueue;
static vector <char*> pipes{};
static vector <pid_t> pids{};

// global variables so they can be handled by signal handler and free memory
static char* path;
char* test;


void sigchdlHandler(int signo){
    int status;
    
    pid_t p = waitpid(-1,&status,WUNTRACED|WCONTINUED);
    if(p < 0 ){
        perror("waitpid error");
        exit(20);
    }
    
    // what caused the signal
    if ( WIFCONTINUED(status) ){
        workersQueue.pop();
    }
    
    else{
         workersQueue.push(p);
    }
}

void catchInterrupt(int signo){
    
    // unlink pipes
    for(unsigned long i = 0; i < pipes.size() ; ++i){
        unlink(pipes[i]);
        delete[] pipes[i];
    }

    for(unsigned long i=0;i<pids.size();++i){
        kill(pids[i],SIGKILL);
    }
    delete[] test;
    delete[] path;

    kill(getpid(),SIGTERM);
}



int main(int argc, char* argv[]){

    

    // handle arguments
    if(argc <= 2){
        //current path
        path = new char[3];
        strcpy(path,"./");
    }
    else if(argc == 3 ){
        path = new char[strlen(argv[2])+1];
        strcpy(path,argv[2]);

    }else{
        cout<< "Passed too many arguments"<<endl;
        exit(-1);
    }

    
    // define signals behavior
    sigset_t managerWorksSignalsSet;
    sigemptyset(&managerWorksSignalsSet);
    static struct sigaction intrpt,chdlact;
    sigaddset(&managerWorksSignalsSet,SIGINT);
    sigaddset(&managerWorksSignalsSet,SIGCHLD);
    sigaddset(&managerWorksSignalsSet,SIGSTOP);
    sigaddset(&managerWorksSignalsSet,SIGKILL);
    intrpt.sa_handler = catchInterrupt;
    sigaction(SIGINT,&intrpt,NULL);
    chdlact.sa_handler = sigchdlHandler;
    sigaction(SIGCHLD,&chdlact,NULL);
    
    

    // unnamed pipe for manager/listener communication
    int managerListenerCommunicationPipe[2];

    if(pipe(managerListenerCommunicationPipe) == -1){
        perror("pipe call");
        exit(1);
    }

    pid_t pid;
    switch (pid = fork()){
    
    // in case there was an error executing fork()
    case -1:
        perror("pipe call");
        exit(2);
    
    // child proccess - Listener - writes to pipe
    case 0:

        // child writes
        if( close(managerListenerCommunicationPipe[READ]) < 0 ){
            perror("close unnamed pipe on listener");
            exit(20);
        }
        // call listener function
        listener(managerListenerCommunicationPipe[WRITE],path);
        break;
        
    // parent proccess - Manager -reads from pipe
    default:

        // parent reads

        if( close(managerListenerCommunicationPipe[WRITE]) < 0 ){
            perror("close unnamed pipe on manager");
            exit(21);
        }

        char inbuf[257];

        // max file name in unix is 256 chars + 1 for '\0'
        char* filename;

        int sofar = 0;

        bool firstTime = true;

        while(true){

            // sofar represents how many bytes we have read from inbuf
            sofar = 0;

            // readble reprepsents how many bytes were read from manager-listener pipe
            int readable;
            // manager communicates with listener

            if(  ((readable = read(managerListenerCommunicationPipe[READ],inbuf,256)) == -1 ) && errno != EINTR ){
                perror("read error");
                kill(getpid(),SIGINT);
            }
            
            // and errno == EINTR
            // continue: go to the next iteration
            if( readable == -1)
                continue;

            // because read does not null terminate buffer, i add "\0" with snprintf
            test = new char[readable+1];

            //null terminate buffer
            inbuf[readable] = '\0';
            snprintf(test,readable+1,"%s",inbuf);

            // in buffer may more than one files appear
            // each file is seperated with the other by '\n'
            // use strtok to retrieve each file name
            filename = strtok(test,"\n");
            
            // sofar: length of first file name
            sofar += strlen(filename);
            
            // if not even the first file name is in buffer, then inbuf[sofar] != '\n'
            if( inbuf[sofar] != '\n' ){

                ofstream f;
                f.open("t1",ios::app);
                f << "lala"<<endl;
                f.close();

                // take the rest bytes of the filename
                // read byte by byte until you read the filename
                char byteChar;
                char* previousStr = new char[sofar];
                strcpy( previousStr , filename );

                while(true){
                    if(((readable = read(managerListenerCommunicationPipe[READ],&byteChar,1)) == -1 ) && errno != EINTR ){
                        perror("read error");
                        kill(getpid(),SIGINT);
                    }
                    // and errno == EINTR
                    if( readable == -1)
                        continue;
                    
                    if(byteChar != '\n'){
                        filename = new char[sofar+1];
                        snprintf(filename,sofar+2,"%s%c",previousStr,byteChar);
                        sofar++;
                        delete[] previousStr;
                        char* previousStr = new char[sofar];
                        strcpy(previousStr,filename); 

                    }else if( byteChar == '\n' ){
                        delete[] previousStr;
                        // to keep with the below while
                        filename = strtok(filename,"\n");
                        break;
                    }


                }

            }



            // strtok
            while ( filename != NULL ){
                
                // if no available worker in queue
                if(workersQueue.empty() || (firstTime == true) ){
                    
                    // fork to create a worker
                    pid_t workerpid = fork(); 

                    // pipe name consist of manager pid and worker pid
                    // for example if manager pid is 1234 and worker pid is 5678 then
                    //  pipe name will by 12345678
                    char* pipe = new char[2*sizeof(pid_t)+3+6];
                    if(workerpid == 0)
                        snprintf(pipe, 2*sizeof(pid_t)+3+6 ,"pipes/%d%d",getppid(),getpid());
                    else{
                        pids.push_back(workerpid);
                        snprintf(pipe, 2*sizeof(pid_t)+3+6,"pipes/%d%d",getpid(),workerpid);}


                    if(  (mkfifo(pipe, PERMS) < 0) && errno != EEXIST ){
                            perror("cant create named pipe");
                            if(workerpid != 0)
                                kill(getpid(),SIGINT);
                            else
                                kill(getppid(),SIGINT);
                            exit(30);
                    }else{
                        // keep pipe names so you can delete them in the end of the execution
                        if(workerpid != 0)
                            pipes.push_back(pipe);
                        else
                            delete[] pipe;
                    }
                      
                    // manager
                    if(workerpid != 0 ){

                        int pipeDesc;

                        //* blocks here till other side  call open
                        if ( (pipeDesc = open(pipe, O_WRONLY)) < 0 ){
                            perror( "cant opeeeen pipe" );
                            kill(getpid(),SIGINT);
                            exit(10);
                        }

                        // write filename in pipe
                        if( write(pipeDesc,filename, strlen(filename)) < 0 ){
                            perror("write to named pipe");
                            close(pipeDesc);
                            kill(getpid(),SIGINT);
                            exit(11);
                        } 

                        // close pipe
                        if(close(pipeDesc)){
                            perror("close pipe");
                            kill(getpid(),SIGINT);
                        }
                    }

                    //  worker
                    if( workerpid == 0){
                        
                        // no busy waiting here, because it blocks with SIGSTOP
                        while(true){
                            
                            // retrieve pipe name using parent pid and your pid
                            char* pipe = new char[2*sizeof(pid_t)+3+6];
                            snprintf(pipe,2*sizeof(pid_t)+3+6,"pipes/%d%d",getppid(),getpid());

                            workerFunc(pipe,path);
                            
                            // worker calls stop      
                            delete[] pipe;               
                            kill(getpid(),SIGSTOP);
                        }
                    }
                                        
                }
                
                // if there is available worker
                else if(!workersQueue.empty()){
                    
                    // retrieve pipe name 
                    char* pipe = new char[2*sizeof(pid_t)+3+6];
                    snprintf(pipe,2*sizeof(pid_t)+3+6,"pipes/%d%d",getpid(),workersQueue.front());

                    ofstream f;
                    f.open("t",ios::app);
                    f << pipe<<endl;
                    f.close();
                    
                    // send signal to worker to continue
                    kill(workersQueue.front(),SIGCONT);

                    int pipeDesc;
                    if ( (pipeDesc = open(pipe, O_WRONLY)) < 0 ){
                        perror( "cant opeeeeen pipe");
                        delete pipe;
                        kill(getpid(),SIGINT);
                        exit(10);
                    }

                    if( write(pipeDesc,filename, strlen(filename)) < 0 ){
                        perror("write to named pipe");
                        delete pipe;
                        close(pipeDesc);
                        kill(getpid(),SIGINT);
                        exit(11);
                    } 

                    delete[] pipe;
                    close(pipeDesc);
                }

                // take next file name, if there exist one
                filename = strtok(NULL,"\n");

                // this if the filename is not completely retrieved from pipe of listener-manager
                if(filename != NULL){
                    
                    //  if not even the first file name is in buffer
                    if( inbuf[sofar] != '\n' ){
                        // take the rest
                        char byteChar;
                        char* previousStr = new char[sofar];
                        strcpy( previousStr , filename );

                        while(true){
                            if(((readable = read(managerListenerCommunicationPipe[READ],&byteChar,1)) == -1 ) && errno != EINTR ){
                                perror("read error");
                            }
                            // and errno == EINTR
                            if( readable == -1)
                                continue;
                            
                            if(byteChar != '\n'){
                                filename = new char[sofar+1];
                                snprintf(filename,sofar+2,"%s%c",previousStr,byteChar);
                                sofar++;
                                delete[] previousStr;
                                char* previousStr = new char[sofar];
                                strcpy(previousStr,filename); 

                            }else if( byteChar == '\n' ){
                                delete[] previousStr;
                                // to keep with the below while
                                filename = strtok(filename,"\n");
                                sofar += strlen(filename);
                                ofstream f;
                                f.open("t2",ios::app);
                                f << filename<<endl;
                                f.close();
                                break;
                            }
                        }
                    }
                }else if(firstTime){
                    firstTime = false;
                }else{
                    delete[] test;
                }
            }
        }
    }

    return 0;
}