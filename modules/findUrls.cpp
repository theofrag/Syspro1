#include <iostream>

#include <map>

#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <fstream>

#include "findUrls.h"

#include <stdio.h>
#include <stdlib.h>

using namespace std;



// it takes a file name and writes to the file filename.out urls from filename and the number of their occurances
//TODO an den yparxei to www. pali to dexomai, afoy yparxei to http://
int findUrls(char* filename, char* listenerPath ,char* outputPath){

    map <string,int> urlMap;
    string http;
    // 
    int fdes; 

    char* listenerFile = new char[strlen(filename) + strlen(listenerPath)+5];
    strcpy(listenerFile,listenerPath);
    strcat(listenerFile,filename);

    

    if( (fdes = open(listenerFile, O_RDWR)) == -1){
        perror("opeen error ");
        exit(3);
    }

    char ch;

    ssize_t readSize;

    while( (readSize = read(fdes,&ch,1)) ){

        
        if(readSize < 0 ){
            perror("read from file");
            exit(4);
        }

        if(ch == 'h'){
            if(read(fdes,&ch,1)<0){
                perror("read from file");
                exit(4);
            }

            if(ch == 't'){

                if(read(fdes,&ch,1)<0){
                    perror("read from file");
                    exit(4);
                }

                if(ch == 't'){
                    
                    if(read(fdes,&ch,1)<0){
                        perror("read from file");
                        exit(4);
                    }

                    if(ch == 'p'){

                        if(read(fdes,&ch,1)<0){
                            perror("read from file");
                            exit(4);
                        }

                        if(ch == ':'){
                            
                            if(read(fdes,&ch,1)<0){
                                perror("read from file");
                                exit(4);
                            }

                            if(ch == '/'){
                                
                                if(read(fdes,&ch,1)<0){
                                    perror("read from file");
                                    exit(4);
                                }

                                if(ch == '/'){

                                    if(read(fdes,&ch,1)<0){
                                        perror("read from file");
                                        exit(4);
                                    }
                                    // ignore www. if exist
                                    if(ch == 'w'){
                                        http += "w";

                                        if(read(fdes,&ch,1)<0){
                                            perror("read from file");
                                            exit(4);
                                        }
                                        if(ch == 'w'){
                                            http += "w";

                                            if(read(fdes,&ch,1)<0){
                                                perror("read from file");
                                                exit(4);
                                            }

                                            if(ch == 'w'){
                                            http += "w";

                                                if(read(fdes,&ch,1)<0){
                                                    perror("read from file");
                                                    exit(4);
                                                }
                                                if(ch == '.'){
                                                    http = "";
                                                }
                                            }
                                        }
                                    }

                                    while(read(fdes,&ch,1)){
                                        if( ch == ' '){

                                            map<string ,int>::iterator it = urlMap.find(http) ;
                                            
                                            if(it == urlMap.end())
                                                urlMap.insert({http,1});
                                            else{
                                                it ->second ++;
                                            }
                                            http.erase();
                                            break;
                                        }

                                        http += ch;
                                    }


                                }
                            }
                        }
                    }
                }
            }

        }


    }
   if(close(fdes) < 0){
       perror("close file");
       exit(5);
   }

    // +4 for .out +1 for '\0' +1 for / in path
    //todo must be freed
    char* outputFile = new char[strlen(filename) + strlen(outputPath )+ 4 + 1];
    
    strcpy(outputFile,outputPath);
    strcat(outputFile,filename);
    strcat(outputFile,".out");
   fdes = open(outputFile,O_CREAT|O_RDWR,S_IRWXU);

   if(fdes == -1){
       perror("file open");
       exit(7);
   }
   
    for( map<string,int>::iterator itr = urlMap.begin(); itr != urlMap.end() ; ++itr ){
        
        // convert a string to char* so i can use write
        int size = itr->first.size();
        char str[size+1];
        strcpy(str,itr->first.c_str());
        char  space = ' ';
        char  newLine = '\n';


        if(write(fdes,str,size+1)<0){
            perror("write error");
            close(fdes);
            delete[] outputFile;
            exit(6);
        }
        if(write(fdes,&space,1)<0){
            perror("write error");
            close(fdes);
            delete[] outputFile;
            exit(6);
        }

        // prints in binary form so ints dont display with cat
        char* number = new char[sizeof(int)+2];
        snprintf(number,sizeof(itr->second)+2,"%d",itr->second);
        if(write(fdes,number,strlen(number))<0){
            perror("write error");
            close(fdes);
            delete[] outputFile;
            exit(6);
        }
        if(write(fdes,&newLine,1)<0){
            perror("write error");
            close(fdes);
            delete[] outputFile;
            exit(6);
        }

    }
    close(fdes);

}

