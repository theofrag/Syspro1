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

        // max file name in unix is 256 chars + 1 for '\0'
        char filename[257];

        int index=0;

        while(true){

            if(read(managerListenerCommunicationPipe[READ],inbuf,1) == -1){
                perror("read error");
            }
            if(inbuf[0] == '\n'){
                filename[index] = '\0';
                cout << filename <<endl;
                index = 0;
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