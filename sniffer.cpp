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
    for(int i = 0; i < pipes.size() ; ++i){
        unlink(pipes[i]);
        delete[] pipes[i];
    }

    kill(0,SIGKILL);
}



int main(int argc, char* argv[]){

    char* path;

    // handle arguments
    if(argc <= 2){
        //current path
        path = new char[3];
        strcpy(path,"./");
    }
    else if(argc == 3 ){
        //TODO path
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

            sofar = 0;
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
            char* test = new char[readable+1];
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

                        // to keep with the below while
                        filename = strtok(filename,"\n");
                        break;
                    }


                }

            }



            // strtok
            while ( filename != NULL ){
                
                // if no available worker in queue
                if(workersQueue.empty() || (firstTime == true)  ){
                    
                    // fork to create a worker
                    pid_t workerpid = fork(); 

                    // pipe name consist of manager pid and worker pid
                    // for example if manager pid is 1234 and worker pid is 5678 then
                    //  pipe name will by 12345678
                    char* pipe = new char[2*sizeof(pid_t)+3];
                    if(workerpid == 0)
                        snprintf(pipe, 2*sizeof(pid_t)+3 ,"%d%d",getppid(),getpid());
                    else
                        snprintf(pipe, 2*sizeof(pid_t)+3,"%d%d",getpid(),workerpid);

                    // create a named pipe.
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
                        //TODO BALE ERROR HANDLING
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
                            char* pipe = new char[2*sizeof(pid_t)+3];
                            snprintf(pipe,2*sizeof(pid_t)+3,"%d%d",getppid(),getpid());

                            workerFunc(pipe,path);
                            
                            // worker calls stop      
                            delete[] pipe;               
                            kill(getpid(),SIGSTOP);
                        }
                    }
                                        
                }
                
                // if there is available worker
                else{
                    
                    // retrieve pipe name 
                    char* pipe = new char[2*sizeof(pid_t)+3];
                    snprintf(pipe,2*sizeof(pid_t)+3,"%d%d",getpid(),workersQueue.front());
                    
                    // send signal to worker to continue
                    kill(workersQueue.front(),SIGCONT);

                    int pipeDesc;
                    if ( (pipeDesc = open(pipe, O_WRONLY)) < 0 ){
                        perror( "cant open pipe");
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
                    sofar += strlen(filename);

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

                                // to keep with the below while
                                filename = strtok(filename,"\n");
                                break;
                            }
                        }
                    }
                }else if(firstTime){
                    firstTime = false;
                }
            }
        }
    }

    return 0;
}