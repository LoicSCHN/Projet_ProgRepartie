#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <iostream>

// Compilation : 
// g++ attachementSHM.cpp -o attachementSHM 

// Run : 
// ./attachementSHM 131072

struct uneChaine{ 
  char c; 
  int x, y;
};

int main(int argc, char * argv[]){
  
  if (argc!=2) {
    printf("Nbre d'args invalide, utilisation :\n");
    printf("%s id_shm\n", argv[0]);
    exit(0);
  }

	
struct uneChaine * p_att;

p_att = (uneChaine *)shmat(atoi(argv[1]), NULL, 0); 

if((void *)p_att == (void *)-1){
  perror("shmat");
}
else {
  std::cout<<"Ok"<<std::endl; 
}

std::cout<<p_att->c<<std::endl; 
std::cout<<p_att->x<<std::endl; 
std::cout<<p_att->y<<std::endl; 

  
return 0;
}
