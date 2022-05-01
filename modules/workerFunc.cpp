#include "common.h"
using namespace std;

void workerFunc(char* pipeName, char* listenerPath){ 

    // open named pipe
    int pipeDesc;
    if( (pipeDesc= open(pipeName, O_RDONLY)) < 0 ){
        perror( "cant open pipe" );
        unlink(pipeName);
        exit(15);
    }
    
    // read file name
    // unix file names can be at most 256 bytes
    // 256 + 1 for null termination of the string
    char fileName[257];
    
    ssize_t t=0;

    if( (t = read(pipeDesc, fileName, 256)) < 0 ){

        perror( "cant read from pipe" );
        close(pipeDesc);
        unlink(pipeName);
        exit(11);
    }
    
    // read doenst terminate with null
    fileName[t] = '\0';
       
    close(pipeDesc);
   


    // call findUrls
    char outPath[6] = "outs/";
    findUrls(fileName, listenerPath ,outPath);

}