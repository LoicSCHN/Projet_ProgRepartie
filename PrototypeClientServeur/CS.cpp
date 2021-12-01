// Compilation : 
// g++ -c CS.cpp && g++ calculCC.o CS.o -o CS -lpthread
// ./CS 6001 192.168.1.64 6002 P1
// ./CS 6002 192.168.1.64 6001 P2

/*** TODO : Trouver le server affiche plusieurs fois un message recus ***/ 

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "calcul.h"

#define MAX_BUFFER_SIZE 16000

int recvTCP(int socket,  const void * buffer, size_t length,
 unsigned int *nbBytesReceived, unsigned int * nbCallRcv, int bloc) {
  int rcvTot = 0;
  int received = 0;
  while (rcvTot < length) {
    received = recv(socket, (char*)(buffer)+rcvTot, bloc, 0);
    if (received <= 0) {
      return received;
    }
    rcvTot += received;
    (*nbBytesReceived)+= received;
    (*nbCallRcv)++;
  }
  return 1;
}

int sendTCP(int socket, const char * buffer, size_t length,
 unsigned int *nbBytesSent, unsigned int * nbCallSend) {
  int sndTot = 0;
  int sent = 0;
  int cpt = 0;
  while (sndTot < length) {
    sent = send(socket, (buffer)+sndTot, length - sndTot, 0);
    if (sent <= 0) {
      return sent;
    }
    sndTot += sent;
    cpt++;

    (*nbBytesSent)+=sent;
    (*nbCallSend)++;
  }
  return 1;
}


struct paramsFonctionThread {
  int idThread;
  char* portS; 
};


void * fonctionThread (void * params){

  struct paramsFonctionThread * args = (struct paramsFonctionThread *) params;


  int ds = socket(PF_INET, SOCK_STREAM, 0);
  if (ds == -1) {
    perror("Serveur : probleme creation socket\n");
    exit(1); 
  }

  //printf("Serveur : creation de la socket : ok\n");

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(args->portS));

  if(bind(ds, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("Serveur : erreur bind");
    close(ds); 
    exit(1); 
  }

  //printf("Serveur : nommage : ok\n");
  int ecoute = listen(ds, 10);
  if (ecoute < 0) {
    printf("Serveur : je suis sourd(e)\n");
    close (ds);
    exit (1);
  }
  //printf("Serveur: mise en écoute : ok\n");
  
  char messagesRecus[MAX_BUFFER_SIZE];
  int name_size = 0;
  int rcv = 0;
  int dsCv = 0;
  
  fd_set set;
  fd_set settmp;
  FD_ZERO(&set);
  FD_SET(ds, &set);
  
  int max = ds;
  struct sockaddr_in addrC;
  socklen_t lgCv = sizeof(struct sockaddr_in);

  //std::cout<<"Server : Start"<<std::endl; 

  /* boucle pour le traitement itératif des clients */
  while(1){

    settmp = set;
    select(max+1, &settmp, NULL, NULL, NULL);
    /*    fgetc(stdin);*/
    for(int fd = 3; fd <= max; fd++) {

      if (fd == ds) {
        //printf("Serveur : j'attends la demande d'un client (accept) \n");
        dsCv = accept(ds, (struct sockaddr *)&addrC, &lgCv);
        if (dsCv < 0) { 
          perror("Serveur : probleme accept :");
          if(close(dsCv) == -1){
            perror("Serveur : probleme close ");
          }
        } 
        else {
          FD_SET(dsCv, &set);
          if( max < dsCv) {
            max = dsCv;
          }
          //printf("Serveur : le client %s:%d est connecté  \n", inet_ntoa(addrC.sin_addr), htons(server.sin_port));
        }
        //printf("Serveur : Continue\n");
        continue;
      }
      rcv = recv(dsCv, &name_size, sizeof(int), 0);
      /*        printf("Serveur : j'ai reçu au total %d octets avec %d appels à recv \n", nbTotalOctetsRecus, nbAppelRecv);*/
      // taille du nom de fichier
      //printf("Serveur : taille nom de fichier reçue => '%d'\n",name_size);

      rcv = recv(dsCv, messagesRecus, name_size,0);
      
      // nom de fichier
      printf("Serveur : nom de fichier reçue => '%s'\n", messagesRecus);

    }
  }

  close (ds); // atteignable si on sort de la boucle infinie.
  std::cout<<"Serveur : fin"<<std::endl; 

  pthread_exit(NULL);
  return 0; 
}


int main(int argc, char * argv[]){

  if (argc < 5){
    printf("utilisation: %s  numero_port ip_serveur port_serveur nom_fichier\n", argv[0]);
    return 1;
  }     

  srand(time(NULL));

  pthread_t threads;

  struct paramsFonctionThread params; 
 
  params.idThread = 1; 
  params.portS  = argv[1]; 


  if (pthread_create(&threads, NULL, fonctionThread, &params) != 0){
    perror("erreur creation thread");
    exit(1);
  }

  // Client : 
  char* ip_serveur = argv[2]; 
  char* port_serveur = argv[3]; 
  char* nom_fichier = argv[4]; 

  // Mettre un client dans la boucle : 
  while(1){ 
    //std::cout<<"Message à envoyer : "; 
    std::cin>>nom_fichier; 

    int ds = socket(PF_INET, SOCK_STREAM, 0);

    if (ds == -1) {
      printf("Client : pb creation socket\n");
      exit(1); 
    }

    //printf("Client : creation de la socket : ok\n");
    
    struct sockaddr_in adrServ;
    adrServ.sin_addr.s_addr = inet_addr(ip_serveur);
    adrServ.sin_family = AF_INET;
    adrServ.sin_port = htons(atoi(port_serveur));
    socklen_t lgAdr = sizeof(struct sockaddr_in);

    int conn = connect(ds,(struct sockaddr*) &adrServ, lgAdr);
    if (conn <0) {
      perror ("Client: pb au connect :");
      close (ds); 
      exit (1); 
    }  
    //printf("Client : demande de connexion reussie \n");

    unsigned int nbTotOctetsEnvoyes = 0;
    unsigned int nbAppelSend = 0;

    // Envoie de la taille 
    int name_size = strlen(nom_fichier) + 1;
    int snd = sendTCP(ds, (char*)&name_size, sizeof(name_size), &nbTotOctetsEnvoyes, &nbAppelSend);
    if (snd == -1) {
      printf("Client : send n'a pas fonctionné\n");
    }
    //printf("Client : j'ai envoyé au total %d octets avec %d appels à send \n",nbTotOctetsEnvoyes,  nbAppelSend) ;
    //printf("Client : valeur envoyé => '%d'\n", name_size);

    // Envoie du mot clé 
    snd = sendTCP(ds, (char*)nom_fichier, name_size, &nbTotOctetsEnvoyes, &nbAppelSend);
    if (snd == -1) {
      printf("Client : send n'a pas fonctionné\n");
    }
    //printf("Client : j'ai envoyé au total %d octets avec %d appels à send \n", nbTotOctetsEnvoyes,  nbAppelSend) ;
    //printf("Client : valeur envoyé => '%s'\n", nom_fichier);
    
    close (ds);
    //printf("Client : je termine\n");

  }

  pthread_join(threads, NULL); 

  return 0;
 
}
 
