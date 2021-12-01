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

// dig -4 TXT +short o-o.myaddr.l.google.com @ns1.google.com
//

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

  // Envoie de la taille 
  int name_size = strlen(argv[3]) + 1;
  int snd = sendTCP(ds, (char*)&name_size, sizeof(name_size), &nbTotOctetsEnvoyes, &nbAppelSend);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }
  printf("Client : j'ai envoyé au total %d octets avec %d appels à send \n",nbTotOctetsEnvoyes,  nbAppelSend) ;
  printf("Client : valeur envoyé => '%d'\n", name_size);

  // Envoie du mot clé 
  snd = sendTCP(ds, (char*)argv[3], name_size, &nbTotOctetsEnvoyes, &nbAppelSend);
  if (snd == -1) {
    printf("Client : send n'a pas fonctionné\n");
  }
  printf("Client : j'ai envoyé au total %d octets avec %d appels à send \n", nbTotOctetsEnvoyes,  nbAppelSend) ;
  printf("Client : valeur envoyé => '%s'\n", argv[3]);
  
  close (ds);
  printf("Client : je termine\n");
}