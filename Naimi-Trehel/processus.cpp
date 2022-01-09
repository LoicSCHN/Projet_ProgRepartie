// Compilation : 
// g++ -c processus.cpp && g++ calculCC.o processus.o -o processus -lpthread

// -------------------- LOIC ---------------------
// ./processus 172.17.0.143 6001 172.17.0.143 6001
// ./processus 172.17.0.143 6002 172.17.0.143 6001
// ./processus 172.17.0.143 6003 172.17.0.143 6001

// -------------------- LUCAS --------------------
// ./processus 192.168.1.64 6001 192.168.1.64 6001
// ./processus 192.168.1.64 6002 192.168.1.64 6001
// ./processus 192.168.1.64 6003 192.168.1.64 6001

#include <vector>
#include <iostream>
#include <string>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <mutex>
#include "calcul.h"

#define MAX_BUFFER_SIZE 16000

std::mutex mutexOnSHM;

struct threadData {
  bool token; 
  bool demande; 

  char* ip;
  char* port;
  char* ipSuivant;
  char* portSuivant; 
  char* ipPere;
  char* portPere;

  pthread_mutex_t lock; 
  pthread_cond_t tokenSignal;

} data;

std::string getTIME(){
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string s(30, '\0');
    std::strftime(&s[0], s.size(), "%H:%M:%S", std::localtime(&now));
    return s;
}

// Initialisation des locks
void init(struct threadData * b, const bool & start){
  pthread_mutex_init(&(b->lock), NULL);
  pthread_cond_init(&(b->tokenSignal), NULL);
  b->token = start; 
}

// Méthode pour débloquer l'entrer en section critique 
// lors de la reception du token
void getTOKEN(struct threadData *b){
  pthread_mutex_lock(&b->lock);
  std::cout<<"Signal"<<std::endl;
  pthread_cond_signal(&b->tokenSignal);
  pthread_mutex_unlock(&b->lock);
}

// Méthode de controle d'entrer en section critique
void controlSECTION(struct threadData *b){
  if(b->token == false) {
    pthread_mutex_lock(&b->lock);
    pthread_cond_wait(&b->tokenSignal, &b->lock);
    pthread_mutex_unlock(&b->lock);
  }
  
  std::cout<<"Entrer : "<<getTIME()<<std::endl;
  calcul(rand()%4+1); 
  std::cout<<"Sortie : "<<getTIME()<<std::endl;


  b->token = false; 
}

// Envoie de message
void sendMessageTo(char* msg, char* ip, char* port){

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

// Appel dans le main
// Création du thread receveur
// Boucle d'attente de réception du token
void * threadServeur (void * params){
  int param = (long) params;
  int ds = socket(PF_INET, SOCK_STREAM, 0);

  if (ds == -1) {
    perror("Serveur : probleme creation socket\n");
    exit(1); 
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(data.port));

  if(bind(ds, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("Serveur : erreur bind");
    close(ds); 
    exit(1); 
  }

  
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

    // addrC : Contient les infos sur le client
    // On peut connaitre l'adresse ip et la prot du client
    FD_SET(dsCv, &set);
    if( max < dsCv) {
      max = dsCv;
    }
    
    // Reception de la taille du message
    rcv = recv(dsCv, &name_size, sizeof(int), 0);

    // Reception du message 
    rcv = recv(dsCv, messagesRecus, name_size,0);

    std::string msgRCV(messagesRecus); 

    // Prendre le verrou
    mutexOnSHM.lock(); 

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
      if(std::string(data.ipPere) == std::string("") && std::string(data.portPere) == std::string("")){
          if(data.demande == true){
            // suivant := k 
            data.ipSuivant = ipK;    // ip de K
            data.portSuivant = portK;   // port de K; 
          }
          // sinon
          else{
            // jeton-présent := faux; 
            data.token = false; 
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

        sendMessageTo(m, data.ipPere, data.portPere);

      }
      // père := k 
      data.ipPere = ipK; 
      data.portPere = portK; 
      
    }
    else if(msgRCV.at(0) == TOKEN_RECUS.at(0)){
      // ------------------------------------------------------
      //                    RECEPTION TOKEN
      // ------------------------------------------------------
      std::cout<<"Token recus : "<<getTIME()<<std::endl;
      

      // jeton-présent := vrai 
      data.token = true; 

      // GET TOKEN
      getTOKEN(&data); 
    }
    else{
      //Afficher le message recus :
      std::cout<<"Message recus => "<<msgRCV<<std::endl;
    }

    // Rend le verou
    mutexOnSHM.unlock(); 
  }

  close (ds); // atteignable si on sort de la boucle infinie.
  std::cout<<"Serveur : fin"<<std::endl; 

  pthread_exit(NULL);
  return 0; 
}

// Appel dans le main
// Prend le token, rentre en section critique 
// et passe le token au suivant
void* threadCalculateur (void * params){
  int param = (long) params;

  char* msg = strdup("send"); 

  srand(time(NULL));

  // Attachement 
  // struct commonData * p_att;

  // p_att = (commonData *)shmat(param, NULL, 0); 

  // if((void *)p_att == (void *)-1){
  //   perror("shmat");
  // }


  while(1){

    // ------------------------------------------------------
    //          DEMANDE D'ENTRER EN SECTION CRITIQUE
    // ------------------------------------------------------

    // Prendre la verrou
   
    mutexOnSHM.lock(); 

    // demande = true;  
    data.demande = true; 

    // si pere == "" 
    if(std::string(data.ipPere) == std::string("") && std::string(data.portPere) == std::string("")){
      // alors entrée en section critique

    }
    // sinon 
    else{
      //   début envoyer Req(i) à père;
      std::string msgTMP = "D/" + std::string(data.ip) + "/" + std::string(data.port)+ "/"; 

      char *m = new char[msgTMP.length() + 1];
      strcpy(m, msgTMP.c_str());

      sendMessageTo(m, data.ipPere, data.portPere); 

      //   père = "" 
      data.ipPere = strdup(""); 
      data.portPere = strdup("");

    }


    // Rend le verou


    mutexOnSHM.unlock();

    controlSECTION(&data);
    
    // Prendre la verrou
    mutexOnSHM.lock(); 
   
    data.demande = false; 

    // si suivant != "" 
    if(std::string(data.ipSuivant) != std::string("") && std::string(data.portSuivant) != std::string("")){
      //   envoyer token à suivant;
      std::cout<<"Emetteur : Je passe le jeton au suivant"<<std::endl;
      //sendToken(); 
      char* m = strdup("T"); 
      //mutexTOKKEN.unlock(); 
      sendMessageTo(m, data.ipSuivant, data.portSuivant); 
      //   jeton-présent := faux;
      data.token = false; 
      //   suivant := nil 
      data.ipSuivant = strdup("");
      data.portSuivant = strdup(""); 
    }

    // Rend le verou

    mutexOnSHM.unlock(); 

  }
}


// Initialisation, création des threads
int main(int argc, char * argv[]){
  // Vérifier les paramètres
  if (argc < 5){
    printf("utilisation: %s ip port pere_ip pere_port \n", argv[0]);
    return 1;
  }   

  char* ipProcessus = argv[1]; 
  char* portProcessus = argv[2]; 
  char* ipPere = argv[3];
  char* portPere = argv[4];

  bool start = (std::string(ipPere) == std::string(ipProcessus) && std::string(portPere) == std::string(portProcessus)); 

  data.ip = ipProcessus; 
  data.port = portProcessus; 

  // ----------------------- Initialisation
  // père := 1
  data.ipPere = ipPere; 
  data.portPere = portPere; 

  // suivant := null 
  data.ipSuivant = strdup(""); 
  data.portSuivant = strdup(""); 
  
  // demande := faux 
  data.demande = false; 

  // si père = i // Soit meme
  if(start) {
    std::cout<<"Je commence !"<<std::endl; 
    //   alors debut jeton-présent := vrai;
    data.token = true; 
    //   père := null 
    data.ipPere = strdup("");
    data.portPere = strdup("");
  }
  // sinon 
  else { 
    data.token = false; 
  }

  init(&data, start);

  // -------------- Création des deux threads Emetteur/Receveur
  pthread_t threadReceveur, threadEmetteur;

  // Création du thread Receveur 
  if (pthread_create(&threadReceveur, NULL, threadServeur, NULL) != 0){
    perror("erreur creation thread receveur");
    exit(1);
  }

  // Création du thread Emetteur
  if (pthread_create(&threadEmetteur, NULL, threadCalculateur,  NULL) != 0){
    perror("erreur creation thread emetteur");
    exit(1);
  }

  pthread_join(threadReceveur, NULL); 
  pthread_join(threadEmetteur, NULL);

  return 0;
}