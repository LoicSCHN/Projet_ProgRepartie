// Compilation : 
// g++ -c processus.cpp && g++ calculCC.o processus.o -o processus -lpthread

// Naimi Asus  :
// ./processus 1 192.168.1.64 6001 192.168.1.64 6001
// ./processus 2 192.168.1.64 6002 192.168.1.64 6001
// ./processus 3 192.168.1.64 6003 192.168.1.64 6001

// Naimi Asus ROG avec une grosse carte graphique :
// ./processus 1 172.20.176.117 6001 172.20.176.117 6001
// ./processus 2 172.20.176.117 6002 172.20.176.117 6001
// ./processus 3 172.20.176.117 6003 172.20.176.117 6001
// ./processus 4 172.20.176.117 6004 172.20.176.117 6001

#include <vector>
#include <iostream>

#include <string>
#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "calcul.h"

#define MAX_BUFFER_SIZE 16000

struct paramsFonctionThread {
  int idThread;
  int idSEM; 
  int idSHM; 
  
};

struct commonData{ 
  bool token;
  bool demande; 

  char* ip;
  char* port;
  char* ipSuivant;
  char* portSuivant; 
  char* ipPere;
  char* portPere; 
}SHM;


void sendMessageTo(char* msg, char* ip, char* port){
  //std::cout<<"ip : "<<ip<<" port : "<<port<<std::endl;
  // ----------------------- EMETTEUR 
  int ds = socket(PF_INET, SOCK_STREAM, 0);

  if (ds == -1) {
    printf("Client : pb creation socket\n");
    exit(1); 
  }

  struct sockaddr_in adrServ;
  adrServ.sin_addr.s_addr = inet_addr(ip);
  adrServ.sin_family = AF_INET;
  adrServ.sin_port = htons(atoi(port));
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


  // Envoie de la taille 
  int nom_size = strlen(msg) + 1;
 
  int snd = send(ds, (char*)&nom_size, sizeof(nom_size),0);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }


    // Envoie du mot clé 
  snd = send(ds, (char*)msg, nom_size, 0);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }

  close (ds);
  shutdown(ds, SHUT_WR); 
}


void * fonctionThreadReceveur (void * params){
  struct paramsFonctionThread * args = (struct paramsFonctionThread *) params;

  int ds = socket(PF_INET, SOCK_STREAM, 0);

  if (ds == -1) {
    perror("Serveur : probleme creation socket\n");
    exit(1); 
  }

  // Attachement 
  struct commonData * p_att;

  p_att = (commonData *)shmat(args->idSHM, NULL, 0); 

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

  // Structure pour les opérations sur les SEM
  struct sembuf opp;

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

    // Prendre la verrou
    opp.sem_num = 0; // Numéro du sémaphore
    opp.sem_op = -1; // Opération 
    opp.sem_flg = 0; // ??? 
    semop(args->idSEM, &opp, 1);

    if(msgRCV.at(0) == DEMANDE_RECUS.at(0)){
      // ------------------------------------------------------
      //        RECEPTION Req(k)(k est le demandeur)
      // ------------------------------------------------------


      // On récupère l'ip et le port de K : 
      std::vector<std::string> tab(3, ""); 
      std::string delimiter = "/";

      size_t pos = 0;
      int i = 0; 
      std::string token;
      while ((pos = msgRCV.find(delimiter)) != std::string::npos) {
        token = msgRCV.substr(0, pos);
        tab[i] = token;
        msgRCV.erase(0, pos + delimiter.length());
        i++; 
      }

      char *ipK = new char[tab[1].length() + 1];
      strcpy(ipK, tab[1].c_str());

      char *portK = new char[tab[2].length() + 1];
      strcpy(portK, tab[2].c_str());

      // si père = "" 
      if(std::string(p_att->ipPere) == std::string("") && std::string(p_att->portPere) == std::string("")){
          std::cout<<"Demande recus !"<<std::endl; 
          // si demande 
          if(p_att->demande == true){
            // suivant := k 
            p_att->ipSuivant = ipK;//strdup("");    // ip de K
            p_att->portSuivant = portK; //strdup("");  // port de K; 
          }
          // sinon
          else{
            // jeton-présent := faux; 
            p_att->token = false; 
            // envoyer token à k 
            char* tok = strdup("T"); 
            sendMessageTo(tok, ipK, portK);

          } 
        
      }// sinon 
      else {
        // envoyer req(k) à père 
        std::string msgTMP = "D/" + std::string(ipK) + "/" + std::string(portK)+ "/"; 

        char *m = new char[msgTMP.length() + 1];
        strcpy(m, msgTMP.c_str());

        sendMessageTo(m, p_att->ipPere, p_att->portPere);

      }
      // père := k 
      p_att->ipPere = ipK; 
      p_att->portPere = portK; 
      
    }
    else if(msgRCV.at(0) == TOKEN_RECUS.at(0)){
      // ------------------------------------------------------
      //                    RECEPTION TOKEN
      // ------------------------------------------------------
      std::cout<<"Token recus !"<<std::endl; 

      // jeton-présent := vrai 
      p_att->token = true; 

      // Je libère le sémaphore
      opp.sem_num = 1;
      opp.sem_op = 1; 
      opp.sem_flg = 0; 
      semop(args->idSEM, &opp, 1);
    }
    else{
      //Afficher le message recus :
      //printf("Message recus => '%s'\n", messagesRecus);
      std::cout<<"Message recus => "<<msgRCV<<std::endl;
    }

    // Rend le verou
    opp.sem_num = 0;
    opp.sem_op = 1; 
    opp.sem_flg = 0; 
    semop(args->idSEM, &opp, 1);

  }

  close (ds); // atteignable si on sort de la boucle infinie.
  std::cout<<"Serveur : fin"<<std::endl; 

  pthread_exit(NULL);
  return 0; 
}


void* fonctionThreadEmetteur (void * params){
  struct paramsFonctionThread * args = (struct paramsFonctionThread *) params;

  char* msg = strdup("send"); 

  //test(std::string(args->ip), std::string(args->port));
  srand(time(NULL));

  // Attachement 
  struct commonData * p_att;

  p_att = (commonData *)shmat(args->idSHM, NULL, 0); 

  if((void *)p_att == (void *)-1){
    perror("shmat");
  }

  // Structure pour les opérations sur les SEM
  struct sembuf opp;

  while(1){
    // ------------------------------------------------------
    //                        CALCUL
    // ------------------------------------------------------
    calcul(rand()%4+1); 

    // ------------------------------------------------------
    //          DEMANDE D'ENTRER EN SECTION CRITIQUE
    // ------------------------------------------------------

    // Prendre la verrou
    opp.sem_num = 0; // Numéro du sémaphore
    opp.sem_op = -1; // Opération 
    opp.sem_flg = 0; // ??? 
    semop(args->idSEM, &opp, 1);
    std::cout<<"Emetteur : Verrou prit pour faire la demande"<<std::endl;

    // demande = true;  
    p_att->demande = true; 

    // si pere == "" 
    if(std::string(p_att->ipPere) == std::string("") && std::string(p_att->portPere) == std::string("")){
      // alors entrée en section critique
      // ??????

    }
    // sinon 
    else{
      //   début envoyer Req(i) à père;
      std::string msgTMP = "D/" + std::string(p_att->ip) + "/" + std::string(p_att->port)+ "/"; 

      char *m = new char[msgTMP.length() + 1];
      strcpy(m, msgTMP.c_str());

      sendMessageTo(m, p_att->ipPere, p_att->portPere); 

      //   père = "" 
      p_att->ipPere = strdup(""); 
      p_att->portPere = strdup("");

    }

    std::cout<<"Emetteur : Demande faite je rend le verrou"<<std::endl;
    // Rend le verou
    opp.sem_num = 0;
    opp.sem_op = 1; 
    opp.sem_flg = 0; 
    semop(args->idSEM, &opp, 1);

    // J'attend le token
    opp.sem_num = 1;
    opp.sem_op = -1; 
    opp.sem_flg = 0; 
    std::cout<<"Emetteur : J'attend le token"<<std::endl;
    semop(args->idSEM, &opp, 1);
    std::cout<<"Emetteur : J'ai le token ! je rentre en section critique"<<std::endl;

    // ------------------------------------------------------
    //              ENTRER EN SECTION CRITIQUE
    // ------------------------------------------------------
    // Calcule dans la section critique 
    calcul(rand()%4+1); 
    std::cout<<"Emetteur : Section critique fini"<<std::endl;
    // ------------------------------------------------------
    //              LIBERATION DE LA RESOURCE
    // ------------------------------------------------------
    // Prendre la verrou
    opp.sem_num = 0; // Numéro du sémaphore
    opp.sem_op = -1; // Opération 
    opp.sem_flg = 0; // ??? 
    semop(args->idSEM, &opp, 1);
    std::cout<<"Emetteur : Verrou prit pour faire la liberation"<<std::endl;

    // demande = false;
    p_att->demande = false; 

    // si suivant != "" 
    if(std::string(p_att->ipSuivant) != std::string("") && std::string(p_att->portSuivant) != std::string("")){
      //   envoyer token à suivant;
      std::cout<<"Emetteur : Je passe le jeton au suivant"<<std::endl;
      //sendToken(); 
      char* m = strdup("T"); 
      sendMessageTo(m, p_att->ipSuivant, p_att->portSuivant); 
      //   jeton-présent := faux;
      p_att->token = false; 
      //   suivant := nil 
      p_att->ipSuivant = strdup("");
      p_att->portSuivant = strdup(""); 
    }

    // Rend le verou
    opp.sem_num = 0;
    opp.sem_op = 1; 
    opp.sem_flg = 0; 
    semop(args->idSEM, &opp, 1);
    std::cout<<"Emetteur : Liberation faite je rend le verrou"<<std::endl;

    // Envoyer un message à ip/port
    //std::cin>>msg;
    //sendMessageTo(msg, args->ip, args->port); 
  }
}



int main(int argc, char * argv[]){

  // Vérifier les paramètres
  if (argc < 6){
    printf("utilisation: %s  id ip port pere_ip pere_port \n", argv[0]);
    return 1;
  }   

  char * ipProcessus = argv[2]; 
  char * portProcessus = argv[3]; 
  char * ipPere = argv[4];
  char * portPere = argv[5];


  // ----------------------- SEMAPHORE
  int nombreDeSem = 2; 
  int valeurInit = 1; 
  char* pourCle = strdup("pourCle.txt"); 
  int entierPourCle = atoi(argv[1]); 
  
  int cleSEM = ftok(pourCle, entierPourCle);

  int nbSem = nombreDeSem;
  
  // On essaie de se connecter au tableau semaphores
  int idSEM = semget(cleSEM, nbSem, IPC_EXCL | 0666);
  
  // Si il existe pas on le créé
  if(idSEM == -1){
    //Création du tableau de sémaphores
    idSEM = semget(cleSEM, nbSem, IPC_CREAT | IPC_EXCL | 0666);
    // Vérifier si le tableau à bien été créé
    if(idSEM == -1){
      perror("erreur semget : ");
      exit(-1);
    }
  }

  //printf("sem id : %d \n", idSEM);

  // initialisation des sémaphores a la valeur passée en parametre (faire autrement pour des valeurs différentes):
  ushort tabinit[nbSem];

  // TODO : Modifier l'initialisation 
  //for (int i = 0; i < nbSem; i++) 
  // tabinit[i] = valeurInit;

  tabinit[0] = 1; 
  tabinit[1] = (std::string(ipPere) == std::string(ipProcessus) && std::string(portPere) == std::string(portProcessus)) ? 1 : 0;  
 

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

  struct commonData * p_att;

  p_att = (commonData *)shmat(idSHM, NULL, 0); 

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
  struct paramsFonctionThread params; 

  // Allocation des variables pour les paramètres du Receveur
  params.idThread = 1; 
  params.idSEM = idSEM; 
  params.idSHM = idSHM; 

  // Création du thread Receveur 
  if (pthread_create(&threadReceveur, NULL, fonctionThreadReceveur, &params) != 0){
    perror("erreur creation thread receveur");
    exit(1);
  }

  // Création du thread Emetteur
  if (pthread_create(&threadEmetteur, NULL, fonctionThreadEmetteur, &params) != 0){
    perror("erreur creation thread emetteur");
    exit(1);
  }

  pthread_join(threadReceveur, NULL); 
  // pthread_join(threadEmetteur, NULL); 

  return 0;

}