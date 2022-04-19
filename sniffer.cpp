#include <iostream>
#include <unistd.h>

#include <vector>
#include <string>
#include <string.h>
#include <queue>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "findUrls.h"

#include <sys/wait.h>

#include <signal.h>


using namespace std;

#define READ 0
#define WRITE 1

#define PERMS 0666
#include <fstream>




void workerFunc(char* pipeName){ 

    // open named pipe
    
   

    
    int pipeDesc;
    if( (pipeDesc = open(pipeName, O_RDONLY)) < 0 ){
        perror( "cant open pipe" );
        unlink(pipeName);
        exit(15);
    }
    
    // read file name
    char fileName[256];
    ssize_t t=0;
    
    if( (t = read(pipeDesc, fileName, 256)) < 0 ){

        perror( "cant read from pipe" );
        unlink(pipeName);
        exit(11);
    }
    
    // read doenst terminate with null
    fileName[t] = '\0';
    
    

    //unlink file
    
    close(pipeDesc);
    unlink(pipeName);
    


    // call findUrls
    
    findUrls(fileName, "listenerFile/" ,"outs/");

    

    

    // while(1);
}


int main(void){
    
    

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
        cout << getppid()<<endl;
        // child writes
        close(managerListenerCommunicationPipe[READ]);

        // stdout is now the write end of the pipe.
        // So exec writes in the pipe
        if(dup2(managerListenerCommunicationPipe[WRITE],1) < 0 ){
            perror("dup2 error");
            exit(16);
        }

        // exec inotifywait and notify only for create and moved_to events
        // Give only file name which occur the event
        if (execl("/usr/bin/inotifywait","inotifywait", "-m", "-e", "create","-e", "moved_to" , "--format","%f" , "listenerFile",NULL) == -1){
            perror("exec call");
        }

        break;
    

        // parent proccess - Manager -reads 
    default:

        // parent reads
        close(managerListenerCommunicationPipe[WRITE]);

        char inbuf[1];

        // max file name in unix is 256 chars + 1 for '\0'
        char filename[257];

        int index=0;

        queue <int> workersQueue;

        while(true){

            if(read(managerListenerCommunicationPipe[READ],inbuf,1) == -1){
                perror("read error");
            }

            if(inbuf[0] == '\n'){
                filename[index] = '\0';
                index = 0;


                 // if no available worker
                if(workersQueue.empty()){
                    
                    char pipe[strlen(filename)+ 4] = "Pipe";
                    strcat(pipe,filename);

                    // create a named pipe
                    if(  mkfifo(pipe, PERMS) < 0 ){
                            perror("cant create named pipe");
                    }

                    
                    
                    
                    // create a worker with fork
                    int workerpid = fork(); 
                                    

                    // parent
                    if(workerpid != 0 ){
                        int pipeDesc;
                        // wait for other side to call open
                        if ( (pipeDesc = open(pipe, O_WRONLY)) < 0 ){
                            perror( "cant open pipe" );
                            unlink(pipe);
                            exit(10);
                        }


                        if( write(pipeDesc,filename, strlen(filename)) < 0 ){
                            perror("write to named pipe");
                            unlink(pipe);
                            exit(11);
                        } 

                        close(pipeDesc);
                        unlink(pipe);

                    }

                    //  worker
                    if( workerpid == 0){
                        
                        while(true){
                            workerFunc(pipe);
                            
                            // call stop for yourself
                            kill(getpid(),SIGSTOP);
                        }
                    }
                                        
                }
                
                else{

                 pid_t p = workersQueue.front();
                
                //update filename
                // signal continue in p

                }

                continue;

            }
            filename[index++] = inbuf[0];
            
            // if not available worker -->fork
            // from forked worker call findUrls(prnt);
            // if available wake him up and call findUrls(prnt)

        }
        

    }

    

    return 0;
}