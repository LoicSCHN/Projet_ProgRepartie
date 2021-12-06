
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/sem.h>
#include <ostream>
#include <stdlib.h>
#include <vector>

std::vector<std::string> splitDemande(std::string msg){

  std::vector<std::string> rtn(3, ""); 
  
  //std::string s = msg;
  std::string delimiter = "/";

  size_t pos = 0;
  int i = 0; 
  std::string token;
  while ((pos = msg.find(delimiter)) != std::string::npos) {
    token = msg.substr(0, pos);
    rtn[i] = token;
    msg.erase(0, pos + delimiter.length());
    i++; 
  }

  return rtn; 
}


int main(int argc, char * argv[]){
  
  std::vector<std::string> tab = splitDemande("D/192.168.1.67/6000/"); 

  std::cout<<tab[0]<<std::endl; 
  std::cout<<tab[1]<<std::endl;
  std::cout<<tab[2]<<std::endl;

  //std::cout<<tab<<std::endl; 

  return 0;



}