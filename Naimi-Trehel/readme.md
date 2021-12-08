Mettre le pseudo code avec les signatures de fonction ?

------------------------------------------------------
Initialisation  main()

pere = 1;
suivant = "";
demande = false;  

if pere = i // Soit meme
	jeton-présent = true;
	père = ""; 
sinon 
	jeton-présent :=faux 

------------------------------------------------------
Demande de la section critique par le processus i
void* fonctionThreadEmetteur (void * params)
sendMessageTo(m, p_att->ipPere, p_att->portPere)
demande = true;  

si pere == "" 
	alors entrée en section critique (Incrementer le sem)
sinon 
	début envoyer Req(i) à père;
	père = nil 

------------------------------------------------------
Procédure de fin d'utilisation de la section critique
void* fonctionThreadEmetteur (void * params)
demande = false;

si suivant != "" 
	envoyer token à suivant;
	jeton-présent := faux;
	suivant := nil 

------------------------------------------------------
Réception du message Req (k)(k est le demandeur) 
void * fonctionThreadReceveur (void * params)

si père = nil 
	si demande 
		suivant = k 
	sinon 
		début jeton-présent = false; 
		envoyer token à k 
sinon 
	envoyer req(k) à père 

père := k 

------------------------------------------------------
Réception du message Token

jeton-présent = true;  