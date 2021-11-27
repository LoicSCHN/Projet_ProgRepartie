#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 16000 // taille du buffer qui me permet de récupérer le contenu du fichier à recevoir bloc par bloc. Vous pouvez changer cette valeur.


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

int main(int argc, char *argv[])
{

  if (argc<3) {
    printf("utilisation : %s numero_port taille_param_recv\n", argv[0]);
    exit(1);
  }
  
  int ds = socket(PF_INET, SOCK_STREAM, 0);
  if (ds == -1) {
    perror("Serveur : probleme creation socket\n");
    exit(1); 
  }

  printf("Serveur : creation de la socket : ok\n");

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(argv[1]));

  if(bind(ds, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("Serveur : erreur bind");
    close(ds); 
    exit(1); 
  }

  printf("Serveur : nommage : ok\n");
  int ecoute = listen(ds, 10);
  if (ecoute < 0) {
    printf("Serveur : je suis sourd(e)\n");
    close (ds);
    exit (1);
  }
  printf("Serveur: mise en écoute : ok\n");
  
  char messagesRecus[MAX_BUFFER_SIZE];
  int name_size = 0;
  unsigned int nbTotalOctetsRecus = 0;
  unsigned int nbAppelRecv = 0;
  int rcv = 0;
  int dsCv = 0;
  
  fd_set set;
  fd_set settmp;
  FD_ZERO(&set);
  FD_SET(ds, &set);
  
  int max = ds;
  struct sockaddr_in addrC;
socklen_t lgCv = sizeof(struct sockaddr_in);
 /* boucle pour le traitement itératif des clients */
  while(1){

    settmp = set;
    select(max+1, &settmp, NULL, NULL, NULL);
/*    fgetc(stdin);*/
    for(int fd = 3; fd <= max; fd++) {
      if(!FD_ISSET(fd, &settmp)) {
        printf("Serveur : Continue, parce que je n'ai rien trouvé\n");
        continue;
      }
      if (fd == ds) {
        printf("Serveur : j'attends la demande d'un client (accept) \n");
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
          printf("Serveur : le client %s:%d est connecté  \n", inet_ntoa(addrC.sin_addr), htons(server.sin_port));
        }
        printf("Serveur : Continue\n");
        continue;
      }
      rcv = recv(dsCv, &name_size, sizeof(int), 0);
/*        printf("Serveur : j'ai reçu au total %d octets avec %d appels à recv \n", nbTotalOctetsRecus, nbAppelRecv);*/
 // taille du nom de fichier
      printf("Serveur : taille nom de fichier reçue => '%d'\n",name_size);

      rcv = recv(dsCv, messagesRecus, name_size,0);
       /* printf("Serveur : j'ai reçu au total %d octets avec %d appels à recv \n", nbTotalOctetsRecus, nbAppelRecv);*/
 // nom de fichier
      printf("Serveur : nom de fichier reçue => '%s'\n", messagesRecus);

      char* file_name = malloc((name_size*sizeof(char))+1);
      strcpy(file_name, messagesRecus);
      // je construis le chemin vers le fichier à créer.
      char* filepath = malloc(name_size + 16); // ./reception/+nom fichier
      filepath[0] = '\0';
      strcat(filepath, "./reception/");
      strcat(filepath, file_name);
      
      free(file_name);

      FILE* file = fopen(filepath, "wb");
      if(file == NULL){  // servez vous de cet exemple pour le traitement des erreurs. 
        perror("Serveur : erreur ouverture fichier: \n");
        free(filepath);
        close (dsCv);
        continue; // passer au client suivant 
      }
      free(filepath); // je n'en ia plus besoin.
      long int file_size = 0; 
      int contentReceived = 0; // Compte le nombre d'octets du fichier reçu

      rcv = recv(dsCv, &file_size, sizeof(file_size), 0);
      printf("Serveur : taille de fichier reçue => '%ld'\n", file_size);
      // je reçois le contenu progressivement 
      while(contentReceived < file_size){

        char buffer[MAX_BUFFER_SIZE];
        rcv = recv(dsCv, buffer, MAX_BUFFER_SIZE, 0);
        nbAppelRecv++;
        nbTotalOctetsRecus+=strlen(buffer);
       // si pas d'erreurs, j'ai reçu rcv octets. Je dois les écire dans le fichier.
        size_t written = fwrite(buffer, sizeof(char), rcv, file);
        if(written < rcv){
          perror("Serveur : Erreur a l'ecriture du fichier \n");
          break; // je sors de la boucle d'écrture/réception.
        }

        contentReceived += strlen(buffer);
      }

    // fermeture du fichier à la fin de son écriture ou si erreur s'est produite.

      fclose(file);
      printf("Serveur : fin du dialogue avec le client, nombre total d'octets recus : %d,  recus en %d appels a recv \n", nbTotalOctetsRecus, nbAppelRecv );
      nbAppelRecv=0;
      nbTotalOctetsRecus=0;
      close (dsCv);
      if (rcv <= 0) {
        FD_CLR(fd, &set);
        close(fd);
        printf("Serveur : Continue, client déconnecter, je recommence une boucle\n");
        continue;
      }
    }
    //je passe au client suivant.
  }

  close (ds); // atteignable si on sort de la boucle infinie.
  printf("Serveur : je termine\n");
}