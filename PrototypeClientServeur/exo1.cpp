#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <time.h>

#include "calcul.h"

// Version officiel : 
// g++ -c exo1.cpp
// g++ calculCC.o exo1.o -o exo1 -lpthread
// g++ -c exo1.cpp && g++ calculCC.o exo1.o -o exo1 -lpthread && ./exo1 12


struct paramsFonctionThread {
  int idThread;
};


void * fonctionThread (void * params){

  struct paramsFonctionThread * args = (struct paramsFonctionThread *) params;

  int wait = rand()%10+1; 

  pthread_t moi = pthread_self();
  printf("threads : %d  / moi : %ld \n", getpid(), moi); 

  calcul(wait);

  printf("threads : %d  / moi : %ld / Wait : %d \n", getpid(), moi, wait);


  pthread_exit(NULL); 

   

  printf("Exec : fonctionThread");
  return 0; 
}


int main(int argc, char * argv[]){

  if (argc < 2 ){
    printf("utilisation: %s  nombre_threads  \n", argv[0]);
    return 1;
  }     

  srand(time(NULL));

  pthread_t threads[atoi(argv[1])];

  struct paramsFonctionThread params[atoi(argv[1])]; 
 
  
  // création des threards 
  for (int i = 0; i < atoi(argv[1]); i++){

    params[i].idThread = i; 

    // Le passage de paramètre est fortement conseillé (éviter les
    // variables globales).

     //... // compléter pour initialiser les paramètres
    if (pthread_create(&threads[i], NULL, fonctionThread, &params[i]) != 0){
      perror("erreur creation thread");
      exit(1);
    }
    //join

  }

  for(int i = 0; i < atoi(argv[1]); i++)
    pthread_join(threads[i], NULL); 

  return 0;
 
}
 
