// Compilation Lucas : 
// g++ -c CS.cpp && g++ calculCC.o CS.o -o CS -lpthread
//
// Double Run linux : 
// ./CS 6001 192.168.1.64 6002 P1
// ./CS 6002 192.168.1.64 6001 P2
//
// Double Run Mac
// ./CS 6001 192.168.1.57 6002 P1
// ./CS 6002 192.168.1.57 6001 P2
//
// IP Mac : 192.168.1.57
// IP Linux : 192.168.1.64
// 
// Commande Mac : 
// ./CS 6001 192.168.1.64 6002 P1
// Commande Linux : 
// ./CS 6002 192.168.1.57 6001 P1

// Compilation Loic : 
// g++ -c CS.cpp && g++ calculCC.o CS.o -o CS -lpthread
// ./CS 6001 162.38.83.78 6002 P1
// ./CS 6002 162.38.83.78 6001 P2

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
#include <sys/ipc.h>
#include <sys/sem.h>

#include "calcul.h"

#define MAX_BUFFER_SIZE 16000

bool token = false;
bool demande = false; 

char* suivant ;
char* pere = strdup("");

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
  char* portS; 
  int idSem; 
  
};

struct paramsFonctionEmetteur {
  int idThread;
  char* ip; 
  char* port;
  int idSem; 
  
};


void * fonctionThreadReceveur (void * params){
  struct paramsFonctionReceveur * args = (struct paramsFonctionReceveur *) params;

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
    

    //}
  }

  close (ds); // atteignable si on sort de la boucle infinie.
  std::cout<<"Serveur : fin"<<std::endl; 

  pthread_exit(NULL);
  return 0; 
}



void* fonctionThreadEmetteur (void * params){
 struct paramsFonctionEmetteur * args = (struct paramsFonctionEmetteur *) params;

 char* nom_fichier = strdup(""); 

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


  // ----------------------- SEMAPHORE

  int nombreDeSem = 2; 
  int valeurInit = 1; 
  char* pourCle = strdup("pourCle.txt"); 
  int entierPourCle = 1; 
  
  int clesem = ftok(pourCle, entierPourCle);

  int nbSem = nombreDeSem;
  
  // On essaie de se connecter au tableau semaphores
  int idSem=semget(clesem, nbSem, IPC_EXCL | 0600);
  
  // Si il existe pas on le créé
  if(idSem == -1){
    //Création du tableau de sémaphores
    idSem=semget(clesem, nbSem, IPC_CREAT | IPC_EXCL | 0600);
    // Vérifier si le tableau à bien été créé
    if(idSem == -1){
      perror("erreur semget : ");
      exit(-1);
    }
  }

  printf("sem id : %d \n", idSem);


  
  // initialisation des sémaphores a la valeur passée en parametre (faire autrement pour des valeurs différentes ):
 
  ushort tabinit[nbSem];
  for (int i = 0; i < nbSem; i++) tabinit[i] = valeurInit;;
 

  union semun{
    int val;
    struct semid_ds * buf;
    ushort * array;
  } valinit;
  
  valinit.array = tabinit;

  if (semctl(idSem, nbSem, SETALL, valinit) == -1){
    perror("erreur initialisation sem : ");
    exit(1);
  }

  /* test affichage valeurs des sémaphores du tableau */
  valinit.array = (ushort*)malloc(nbSem * sizeof(ushort)); // pour montrer qu'on récupère bien un nouveau tableau dans la suite

  if (semctl(idSem, nbSem, GETALL, valinit) == -1){
    perror("erreur initialisation sem : ");
    exit(1);
  } 
   
  printf("Valeurs des sempahores apres initialisation [ "); 
  for(int i=0; i < nbSem-1; i++){
    printf("%d, ", valinit.array[i]);
  }
  printf("%d ] \n", valinit.array[nbSem-1]);

  free(valinit.array);
// ----------------------- Fin SEMAPHORE-------------------------------------


  if (argc < 4){
    printf("utilisation: %s  numero_port ip_serveur port_serveur \n", argv[0]);
    return 1;
  }     

  // ----------------------- Initialisation
  // Initialisation

  // père := 1 
  // suivant := nil 
  // demande := faux 
  // si père = i // Soit meme
  //   alors debut jeton-présent := vrai;
  //   père := nil 
  //   fin 
  // sinon 
  //   jeton-présent :=faux 
  // finsi


  // ----------------------- Création des deux threads Emetteur/Receveur
  pthread_t threadReceveur;
  pthread_t threadEmetteur;

  // Déclaration des structures pour passer les paramètres aux deux threads
  struct paramsFonctionReceveur paramsReceveur; 
  struct paramsFonctionEmetteur paramsEmetteur; 

  // Allocation des variables pour les paramètres du Receveur
  paramsReceveur.idThread = 1; 
  paramsReceveur.portS  = argv[1];
  paramsReceveur.idSem = idSem; 
  
  // Allocation des variables pour les paramètres de l'Emetteur
  paramsEmetteur.idThread = 2;
  paramsEmetteur.ip = argv[2];
  paramsEmetteur.port = argv[3];
  paramsEmetteur.idSem = idSem; 

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