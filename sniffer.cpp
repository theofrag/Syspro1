#include <iostream>
#include <unistd.h>

#include <vector>
#include <string>


using namespace std;

#define READ 0
#define WRITE 1


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

        // child writes
        close(managerListenerCommunicationPipe[READ]);
        cout << "before exec"<<endl;
        // stdout is now the write end of the pipe.
        // So exec writes in the pipe
        dup2(managerListenerCommunicationPipe[WRITE],1);

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
        string prnt;
        // char prntbuffer[100];   tha balo vector
        while(1){

            int rsize = read(managerListenerCommunicationPipe[READ],inbuf,1);
            if(inbuf[0] == '\n'){
                cout << prnt<<endl;
                //tha bazo se mia oura to onoma tou arxeiou
                prnt.erase();
                continue;
            }
            prnt += inbuf[0];
            
            // if not available worker -->fork
            // from forked worker call findUrls(prnt);
            // if available wake him up and call findUrls(prnt)

        }
        

    }

    

    return 0;
}