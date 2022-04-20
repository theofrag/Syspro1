#include "common.h"
# include "listener.h"

void listener(int fd){

    // stdout is now the write end of the pipe.
    // So exec writes in the pipe
    if(dup2(fd,1) < 0 ){
        perror("dup2 error");
        exit(16);
    }

    // exec inotifywait and notify only for create and moved_to events
    // Give only file name which occur the event
    if (execl("/usr/bin/inotifywait","inotifywait", "-m", "-e", "create","-e", "moved_to" , "--format","%f" , "listenerFile",NULL) == -1){
        perror("exec call");
    }

}