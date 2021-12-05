// Compilation Lucas : 
// g++ -c processus.cpp && g++ calculCC.o processus.o -o processus -lpthread

// Double Run Mac mini  :
// ./processus 192.168.1.57 6001 192.168.1.57 6002
// ./processus 192.168.1.57 6002 192.168.1.57 6001

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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

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
    //std::cout<<"send"<<std::endl;
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


struct paramsFonctionReceveur {
  int idThread;
  int idSEM; 
  int idSHM; 
  
};

struct paramsFonctionEmetteur {
  int idThread;
  char* ip; 
  char* port;
  int idSEM; 
  int idSHM; 
  
};

struct uneChaine{ 
  bool token;
  bool demande; 

  char* ip;
  char* port;
  char* ipSuivant;
  char* portSuivant; 
  char* ipPere;
  char* portPere; 
}SHM;



void * fonctionThreadReceveur (void * params){
  struct paramsFonctionReceveur * args = (struct paramsFonctionReceveur *) params;

  int ds = socket(PF_INET, SOCK_STREAM, 0);

  if (ds == -1) {
    perror("Serveur : probleme creation socket\n");
    exit(1); 
  }

  // Attachement 
  struct uneChaine * p_att;

  p_att = (uneChaine *)shmat(args->idSHM, NULL, 0); 

  if((void *)p_att == (void *)-1){
    perror("shmat");
  }

  // std::cout<<p_att->token<<std::endl; 
  // std::cout<<p_att->demande<<std::endl; 
  // std::cout<<p_att->ip<<std::endl; 
  // std::cout<<p_att->port<<std::endl; 
  // std::cout<<p_att->ipSuivant<<std::endl; 
  // std::cout<<p_att->portSuivant<<std::endl; 
  // std::cout<<p_att->ipPere<<std::endl; 
  // std::cout<<p_att->portPere<<std::endl; 

  //printf("Serveur : creation de la socket : ok\n");

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  //server.sin_port = htons(atoi(args->portS));
  server.sin_port = htons(atoi(p_att->port));

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
  
  char messagesRecus[MAX_BUFFER_SIZE];
  std::string DEMANDE_RECUS = "D";
  std::string TOKEN_RECUS = "T";
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

  /* boucle de traitement des messages recus */
  while(1){

    settmp = set;
    select(max+1, &settmp, NULL, NULL, NULL);

    // Accepter les message recus : 
    dsCv = accept(ds, (struct sockaddr *)&addrC, &lgCv);

    // ??
    FD_SET(dsCv, &set);
    if( max < dsCv) {
      max = dsCv;
    }
    
    // Reception de la taille du message
    rcv = recv(dsCv, &name_size, sizeof(int), 0);

    // Reception du message 
    rcv = recv(dsCv, messagesRecus, name_size,0);

    std::string msgRCV(messagesRecus); 

    if(msgRCV == DEMANDE_RECUS){
      // ------------------------------------------------------
      // Réception du message Req (k)(k est le demandeur)

      // si père = nil 
      //   alors 
      //   si demande 
      //     alors suivant := k 
      //   sinon 
      //     début jeton-présent := faux; 
      //     envoyer token à k 
      //   fin 
      //   finsi 
      // sinon envoyer req(k) à père 
      // finsi; 
      // père := k 
      std::cout<<"Demande recus !"<<std::endl; 
    }
    else if(msgRCV == TOKEN_RECUS){
      // ------------------------------------------------------
      // Réception du message Token

      // jeton-présent := vrai 
      std::cout<<"Token recus !"<<std::endl; 
    }
    else{
      //Afficher le message recus :
      //printf("Message recus => '%s'\n", messagesRecus);
      std::cout<<"Message recus => "<<msgRCV<<std::endl;
    }
  }

  close (ds); // atteignable si on sort de la boucle infinie.
  std::cout<<"Serveur : fin"<<std::endl; 

  pthread_exit(NULL);
  return 0; 
}


void* fonctionThreadEmetteur (void * params){
 struct paramsFonctionEmetteur * args = (struct paramsFonctionEmetteur *) params;

 char* nom_fichier = strdup(""); 

  // Attachement 
  struct uneChaine * p_att;

  p_att = (uneChaine *)shmat(args->idSHM, NULL, 0); 

  if((void *)p_att == (void *)-1){
    perror("shmat");
  }

  // std::cout<<p_att->token<<std::endl; 
  // std::cout<<p_att->demande<<std::endl; 
  // std::cout<<p_att->ip<<std::endl; 
  // std::cout<<p_att->port<<std::endl; 
  // std::cout<<p_att->ipSuivant<<std::endl; 
  // std::cout<<p_att->portSuivant<<std::endl; 
  // std::cout<<p_att->ipPere<<std::endl; 
  // std::cout<<p_att->portPere<<std::endl; 

 while(1){
  std::cin>>nom_fichier; 


  int ds = socket(PF_INET, SOCK_STREAM, 0);
  //std::cout<<"ds : "<<ds<<std::endl; 

  if (ds == -1) {
    printf("Client : pb creation socket\n");
    exit(1); 
  }

  struct sockaddr_in adrServ;
  adrServ.sin_addr.s_addr = inet_addr(args->ip);
  adrServ.sin_family = AF_INET;
  adrServ.sin_port = htons(atoi(args->port));
  socklen_t lgAdr = sizeof(struct sockaddr_in);

  int conn = -1;

  if(conn == -1){
    conn = connect(ds,(struct sockaddr*) &adrServ, lgAdr);
    if (conn <0) {
      perror ("Client: pb au connect :");
      close (ds); 
      exit (1); 
    }
  }

  unsigned int nbTotOctetsEnvoyes = 0;
  unsigned int nbAppelSend = 0;

    // Envoie de la taille 
  int nom_size = strlen(nom_fichier) + 1;
  int snd = sendTCP(ds, (char*)&nom_size, sizeof(nom_size), &nbTotOctetsEnvoyes, &nbAppelSend);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }


    // Envoie du mot clé 
  snd = sendTCP(ds, (char*)nom_fichier, nom_size, &nbTotOctetsEnvoyes, &nbAppelSend);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }

  close (ds);
  shutdown(ds, SHUT_WR); 
  }
}



int main(int argc, char * argv[]){

  // Vérifier les paramètres
  if (argc < 5){
    printf("utilisation: %s  ip port pere_ip pere_port \n", argv[0]);
    return 1;
  }   

  char * ipProcessus = argv[1]; 
  char * portProcessus = argv[2]; 
  char * ipPere = argv[3];
  char * portPere = argv[4];  


  // ----------------------- SEMAPHORE
  int nombreDeSem = 2; 
  int valeurInit = 1; 
  char* pourCle = strdup("pourCle.txt"); 
  int entierPourCle = 1; 
  
  int cleSEM = ftok(pourCle, entierPourCle);

  int nbSem = nombreDeSem;
  
  // On essaie de se connecter au tableau semaphores
  int idSEM = semget(cleSEM, nbSem, IPC_EXCL | 0600);
  
  // Si il existe pas on le créé
  if(idSEM == -1){
    //Création du tableau de sémaphores
    idSEM = semget(cleSEM, nbSem, IPC_CREAT | IPC_EXCL | 0600);
    // Vérifier si le tableau à bien été créé
    if(idSEM == -1){
      perror("erreur semget : ");
      exit(-1);
    }
  }

  //printf("sem id : %d \n", idSEM);

  // initialisation des sémaphores a la valeur passée en parametre (faire autrement pour des valeurs différentes):
  ushort tabinit[nbSem];
  for (int i = 0; i < nbSem; i++) tabinit[i] = valeurInit;;
 

  union semun{
    int val;
    struct semid_ds * buf;
    ushort * array;
  } valinit;
  
  valinit.array = tabinit;

  if (semctl(idSEM, nbSem, SETALL, valinit) == -1){
    perror("erreur initialisation sem : ");
    exit(1);
  }

  /* test affichage valeurs des sémaphores du tableau */
  valinit.array = (ushort*)malloc(nbSem * sizeof(ushort)); // pour montrer qu'on récupère bien un nouveau tableau dans la suite

  if (semctl(idSEM, nbSem, GETALL, valinit) == -1){
    perror("erreur initialisation sem : ");
    exit(1);
  } 
   
  printf("Valeurs des sempahores apres initialisation [ "); 
  for(int i=0; i < nbSem-1; i++){
    printf("%d, ", valinit.array[i]);
  }
  printf("%d ] \n", valinit.array[nbSem-1]);

  free(valinit.array);

  // ----------------------- SHM 
  int clesSHM = ftok(pourCle, entierPourCle);

  int idSHM = shmget(clesSHM, size_t(sizeof(SHM)) , IPC_CREAT | 0666);
  
  if(idSHM == -1){
    perror("erreur semget : ");
    exit(-1);
  }

  //printf("sem id : %d \n", idSHM);

  struct uneChaine * p_att;

  p_att = (uneChaine *)shmat(idSHM, NULL, 0); 

  if((void *)p_att == (void *)-1){
    perror("shmat");
  }

  p_att->ip = ipProcessus; 
  p_att->port = portProcessus; 

  // ----------------------- Initialisation
  // père := 1
  p_att->ipPere = ipPere; 
  p_att->portPere = portPere; 

  // suivant := nil 
  p_att->ipSuivant = strdup(""); 
  p_att->portSuivant = strdup(""); 
  
  // demande := faux 
  p_att->demande = false;

  // si père = i // Soit meme
  if(std::string(ipPere) == std::string(ipProcessus) && std::string(portPere) == std::string(portProcessus)) {
    std::cout<<"Je commence !"<<std::endl; 
    //   alors debut jeton-présent := vrai;
    p_att->token = true; 
    //   père := null 
    p_att->ipPere = strdup(""); 
    p_att->portPere = strdup(""); 
  }
  // sinon 
  else {
    std::cout<<"Je commence pas !"<<std::endl; 
    // jeton-présent :=faux 
    p_att->token = false;
  }


  // ----------------------- Création des deux threads Emetteur/Receveur
  pthread_t threadReceveur;
  pthread_t threadEmetteur;

  // Déclaration des structures pour passer les paramètres aux deux threads
  struct paramsFonctionReceveur paramsReceveur; 
  struct paramsFonctionEmetteur paramsEmetteur; 

  // Allocation des variables pour les paramètres du Receveur
  paramsReceveur.idThread = 1; 
  //paramsReceveur.portS  = portProcessus;
  paramsReceveur.idSEM = idSEM; 
  paramsReceveur.idSHM = idSHM; 
  
  // Allocation des variables pour les paramètres de l'Emetteur
  paramsEmetteur.idThread = 2;
  paramsEmetteur.ip = ipPere;
  paramsEmetteur.port = portPere;
  paramsEmetteur.idSEM = idSEM; 
  paramsEmetteur.idSHM = idSHM; 

  // Création du thread Receveur 
  if (pthread_create(&threadReceveur, NULL, fonctionThreadReceveur, &paramsReceveur) != 0){
    perror("erreur creation thread receveur");
    exit(1);
  }

  // Création du thread Emetteur
  if (pthread_create(&threadEmetteur, NULL, fonctionThreadEmetteur, &paramsEmetteur) != 0){
    perror("erreur creation thread emetteur");
    exit(1);
  }

  pthread_join(threadReceveur, NULL); 
  // pthread_join(threadEmetteur, NULL); 

  return 0;

}