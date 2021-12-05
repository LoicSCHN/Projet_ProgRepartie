#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <iostream>

// Compilation : 
// g++ createSHM.cpp -o createSHM 

// Run : 
// ./createSHM pourCle.txt 1

struct uneChaine{ 
  char c; 
  int x, y;
}SHM;

int main(int argc, char * argv[]){
  
  if (argc!=3) {
    printf("Nbre d'args invalide, utilisation :\n");
    printf("%s fichier-pour-cle-ipc entier-pour-clé-ipc\n", argv[0]);
    exit(0);
  }
	  
  int clesSHM = ftok(argv[1], atoi(argv[2]));

  int idSHM = shmget(clesSHM, size_t(sizeof(SHM)) , IPC_CREAT | 0666);
  
  if(idSHM == -1){
    perror("erreur semget : ");
    exit(-1);
  }

  printf("sem id : %d \n", idSHM);

  struct uneChaine * p_att;

  p_att = (uneChaine *)shmat(idSHM, NULL, 0); 

  if((void *)p_att == (void *)-1){
    perror("shmat");
  }
  else {
    std::cout<<"Ok"<<std::endl; 
    p_att->c = 'a';
    p_att->x = 18984;
    p_att->y = 27;  
  }

 

  
  return 0;
}
