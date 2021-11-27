#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 16000  // taille du buffer temporaire pour la lecture de fichier. Vous pouvez définir une autre valeur.

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

int main(int argc, char *argv[]) {

  if (argc != 4){
    printf("utilisation : client ip_serveur port_serveur nom_fichier\n");
    exit(0);
  }
  int ds = socket(PF_INET, SOCK_STREAM, 0);

  if (ds == -1) {
    printf("Client : pb creation socket\n");
    exit(1); 
  }

  printf("Client : creation de la socket : ok\n");
  
  struct sockaddr_in adrServ;
  adrServ.sin_addr.s_addr = inet_addr(argv[1]);
  adrServ.sin_family = AF_INET;
  adrServ.sin_port = htons(atoi(argv[2]));
  socklen_t lgAdr = sizeof(struct sockaddr_in);

  int conn = connect(ds,(struct sockaddr*) &adrServ, lgAdr);
  if (conn <0) {
    perror ("Client: pb au connect :");
    close (ds); 
    exit (1); 
  }  
  printf("Client : demande de connexion reussie \n");

  unsigned int nbTotOctetsEnvoyes = 0;
  unsigned int nbAppelSend = 0;

  int name_size = strlen(argv[3]) + 1;
  int snd = sendTCP(ds, (char*)&name_size, sizeof(name_size), &nbTotOctetsEnvoyes, &nbAppelSend);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }
  printf("Client : j'ai envoyé au total %d octets avec %d appels à send \n",nbTotOctetsEnvoyes,  nbAppelSend) ;
  printf("Client : valeur envoyé => '%d'\n", name_size);

  snd = sendTCP(ds, (char*)argv[3], name_size, &nbTotOctetsEnvoyes, &nbAppelSend);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }
  printf("Client : j'ai envoyé au total %d octets avec %d appels à send \n", nbTotOctetsEnvoyes,  nbAppelSend) ;
  printf("Client : valeur envoyé => '%s'\n", argv[3]);

  // je construis le nom complet (chemin) du fichier que je dois lire
  char* filepath = malloc(strlen(argv[3]) + 16); // ./emission/+nom fichier +\0
  filepath[0] = '\0';
  strcat(filepath, "./emission");
  strcat(filepath, argv[3]);

  // je récupère la taille du fichier. Pourquoi ?
  struct stat attributes;
  if(stat(filepath, &attributes) == -1){
    perror("Client : erreur stat");
    free(filepath);
    close(ds);
    exit(1);
  }

  long int file_size = attributes.st_size; 
  // pour envoyer le contenu, je dois lire le fichier :
  FILE* file = fopen(filepath, "rb"); // ouverture en lecture
  if(file == NULL){
    perror("Client : erreur ouverture fichier \n");
    free(filepath);
    close(ds);
    exit(1);   
  }
  free(filepath); // je n'ai plus besoin de ce tableau dans la suite.

  // je fait une lecture par bloc.
  // je continue à lire tant que je n'ai pas lus le fichier en entier.
  snd = sendTCP(ds, (char*)&file_size,sizeof(file_size), &nbTotOctetsEnvoyes, &nbAppelSend);
  printf("Serveur : valeur envoyé => '%ld'\n",file_size);
  nbTotOctetsEnvoyes = 0;
  nbAppelSend=0;

  int nbRead = 0;  //
  while(nbRead < file_size){

    char buffer[MAX_BUFFER_SIZE];
    size_t read = fread(buffer, sizeof(char), MAX_BUFFER_SIZE, file);
    // je viens de tenter de lire un bloc d'au maximum MAX_BUFFER_SIZE octets
    if(read == 0){
      if(ferror(file) != 0){
        perror("Client : erreur lors de la lecture du fichier \n");
        fclose(file);
        exit(1);
      } // fin du fichier
      break; // plus rien à lire
    }
    snd = send(ds, (char*)buffer, MAX_BUFFER_SIZE, 0);
    if (snd == -1) {
      printf("Client : send n'a pas fonctionné\n");
    }
    nbAppelSend++;
    nbTotOctetsEnvoyes+=snd;
    nbRead += MAX_BUFFER_SIZE;
  }
  // fermeture du fichier
  int resc = fclose(file);
  if(resc != 0){
    perror("Client : erreur fermeture fichier\n");
    exit(1);
  }
  printf("Client : j'ai envoye au total : %d octets,  envoyes en %d appels a send \n", nbTotOctetsEnvoyes, nbAppelSend);  
  close (ds);
  printf("Client : je termine\n");
}