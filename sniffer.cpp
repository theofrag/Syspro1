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

void conHandler(int signo){
    workersQueue.pop();
}

void sigchdlHandler(int signo){
    
    pid_t p = waitpid(-1,NULL,WUNTRACED);
    workersQueue.push(p);


}


int main(void){
    
    // define signals behavior
    sigset_t managerWorksSignalsSet;
    sigemptyset(&managerWorksSignalsSet);
    static struct sigaction conact,chdlact;
    sigaddset(&managerWorksSignalsSet,SIGCONT);
    sigaddset(&managerWorksSignalsSet,SIGCHLD);
    sigaddset(&managerWorksSignalsSet,SIGSTOP);
    conact.sa_handler = conHandler;
    sigaction(SIGCONT,&conact,NULL);

    chdlact.sa_handler = sigchdlHandler;
    sigaction(SIGCHLD,&chdlact,NULL);


    

    // buffer for pipe read/write
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
    
    // child proccess - Listener - writes
    case 0:

        // child writes
        close(managerListenerCommunicationPipe[READ]);

        // call listener function
        listener(managerListenerCommunicationPipe[WRITE]);

        break;
    
    // parent proccess - Manager -reads 
    default:

        // parent reads
        close(managerListenerCommunicationPipe[WRITE]);

        char inbuf[256];

        // max file name in unix is 256 chars + 1 for '\0'
        char* filename;

        int sofar = 0;

        while(true){
            
            int readable;
            // manager communicates with listener

            if(  ((readable = read(managerListenerCommunicationPipe[READ],inbuf,256)) == -1 ) && errno != EINTR ){
                perror("read error");
            }

            // and errno == EINTR
            if( readable == -1)
                continue;


            // read does not null terminate
            char test [readable+1]="";
            strcpy(test,inbuf);

            filename = strtok(test,"\n");

            sofar += strlen(filename);


            while ( filename != NULL ){
                
                 // if no available worker in queue
                if(workersQueue.empty()){
                    
                    
                    pid_t workerpid = fork(); 
                    

                    char* pipe = new char[2*sizeof(pid_t)+3];
                    if(workerpid == 0)
                        snprintf(pipe, 2*sizeof(pid_t)+3 ,"%d%d",getppid(),getpid());
                    else
                        snprintf(pipe, 2*sizeof(pid_t)+3,"%d%d",getpid(),workerpid);
                    // create a named pipe. Pipe name is concat of parendpid and workerpid
                    if(  (mkfifo(pipe, PERMS) < 0) && errno != EEXIST ){
                            perror("cant create named pipe");
                    }
                      
                    // parent
                    if(workerpid != 0 ){
                        
                        
                        
                        int pipeDesc;


                        // blocks till other side to call open
                        if ( (pipeDesc = open(pipe, O_WRONLY)) < 0 ){
                            perror( "cant opeeeen pipe" );
                            unlink(pipe);
                            exit(10);
                        }


                        if( write(pipeDesc,filename, strlen(filename)) < 0 ){
                            perror("write to named pipe");
                            unlink(pipe);
                            exit(11);
                        } 

                        close(pipeDesc);
                        // unlink(pipe);


                    }
                    //  worker
                    if( workerpid == 0){
                        while(true){

                            char* pipe = new char[2*sizeof(pid_t)+3];
                            snprintf(pipe,2*sizeof(pid_t)+3,"%d%d",getppid(),getpid());

                            workerFunc(pipe);
                            
                            // call stop for yourself
                            
                            kill(getpid(),SIGSTOP);
                        }
                    }
                                        
                }
                
                else{
                    
                    
                    kill(workersQueue.front(),SIGCONT);

                }

                filename = strtok(NULL,"\n");

            }
        }
        

    }

    

    return 0;
}