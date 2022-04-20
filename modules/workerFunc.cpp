#include "common.h"
using namespace std;

void workerFunc(char* pipeName){ 

    // open named pipe
    int pipeDesc;
    if( (pipeDesc= open(pipeName, O_RDONLY)) < 0 ){
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

    // ofstream f;
    // f.open("t2");
    // f<<fileName;
    // f.close();
    
    

    //unlink file
    
    close(pipeDesc);
    // unlink(pipeName);
    


    // call findUrls
    
    findUrls(fileName, "listenerFile/" ,"outs/");

    

    

    // while(1);
}