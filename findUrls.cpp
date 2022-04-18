#include <iostream>

#include <map>

#include <string.h>
#include <fstream>

#include "findUrls.h"

using namespace std;


// it takes a file name and writes to the file filename.out urls from filename and the number of their occurances
//TODO an den yparxei to www. pali to dexomai, afoy yparxei to http://
int findUrls(string filename){

    map <string,int> urlMap;
    

    ifstream file(filename);
    string outfileName = "./outs/" + filename + ".out";
    
    ofstream outfile(outfileName);

    char ch;
    string http;
    while(file.get(ch)){
        if(ch == 'h'){
            file.get(ch);
            if(!ch) break;
                
            if(ch == 't'){
                file.get(ch);
                if(!ch) break;

                if(ch == 't'){
                    file.get(ch);
                    if(!ch) break;

                    if(ch == 'p'){
                        file.get(ch);
                        if(!ch) break;
                        
                        if(ch == ':'){
                            file.get(ch);
                            if(!ch) break;

                            if(ch == '/'){
                                file.get(ch);
                                if(!ch) break;

                                if(ch == '/'){
                                    
                                    // ignore www.
                                    for(int i=0;i<4;++i){
                                        file.get(ch);
                                    }

                                    while(file.get(ch)){
                                        
                                        if( ch == ' '){
                                            
                                            map<string,int>::iterator it = urlMap.find(http) ;
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

    
    for( map<string,int>::iterator itr = urlMap.begin(); itr != urlMap.end() ; ++itr ){
        outfile << itr -> first << " and "<< itr->second<<endl;
    }

}
