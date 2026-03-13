// Définit le niveau de conformité POSIX pour s'assurer que les fonctions réseau sont disponibles (getaddrinfo et les sockets modernes)
#define _POSIX_C_SOURCE 200112L

#include <string.h>           // Pour std::string
#include <unistd.h>         // Pour close()
#include <sys/types.h>      // Pour ssize_t
#include <sys/socket.h>     // Pour socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h>     // Pour struct sockaddr_in et struct sockaddr_in6
#include <arpa/inet.h>      // Pour inet_ntop()
#include <netdb.h>          // Pour getaddrinfo() et freeaddrinfo()
#include <fcntl.h>          // Pour fcntl()
#include <map>              // Pour std::map
#include <ctime>            // Pour time() et difftime()
#include <iostream>         // Pour std::cout et std::cerr
#include "Client.hpp"       // Pour la classe Client qui gère les connexions individuelles
#include "SystemError.hpp"  // Pour la classe SystemError qui encapsule les erreurs système (errno)
#include "GaiError.hpp"     // Pour la classe GaiError qui encapsule les erreurs de getaddrinfo (gai_strerror)

#define PORT "8080"         // Le port sur lequel le serveur écoutera les connexions
#define BACKLOG 10          // La taille de la file d'attente pour les connexions entrantes en attente
#define MAX_TIMEOUT 10      // Maximum allowed inactivity time for clients (in seconds)


int main(void)
{
	struct addrinfo         hints;                          // Structure pour affiner les résultats de getaddrinfo
	struct addrinfo         *res;                           // Pointeur vers la liste chaînée des résultats de getaddrinfo
	struct addrinfo         *p;                             // Pointeur pour itérer sur la liste 'res'
	int                     status;                         // Stocke le code de retour de getaddrinfo pour vérifier les erreurs
	char                    ip_buffer[INET6_ADDRSTRLEN];    // Tampon pour stocker l'adresse IP sous forme de chaîne (assez grand pour IPv6)
	int                     sockfd = -1;                    // Le file descriptor du socket d'écoute principal (initialisé à -1 pour indiquer qu'il n'est pas encore créé)
	int                     yes = 1;                        // Variable utilisée pour setsockopt() afin de permettre la réutilisation de l'adresse (évite "Address already in use" lors du redémarrage rapide du serveur)

	const std::string   port_str = PORT;                    // Conversion du port en std::string pour plus de flexibilité

	try {
		// --- Préparation des indices (hints) pour getaddrinfo ---
		memset(&hints, 0, sizeof hints);   // Initialise la structure hints avec des zéros par sécurité
		hints.ai_family = AF_UNSPEC;            // Accepte à la fois IPv4 (AF_INET) et IPv6 (AF_INET6)
		hints.ai_socktype = SOCK_STREAM;        // Spécifie qu'on veut une socket TCP (flux de données fiable)
		hints.ai_flags = AI_PASSIVE;            // Indique que l'IP locale doit être remplie automatiquement (pour bind)

		// Récupération des adresses locales possibles pour s'y lier (bind)
		// Cette fonction traduit un nom d'hôte et un port en structures compréhensibles par les sockets.
		// Elle alloue dynamiquement une liste chaînée de configurations possibles.
		// 
		// Paramètres :
		// 1. node (NULL) : En tant que serveur (avec le flag AI_PASSIVE), NULL demande au système 
		//    de nous attribuer notre propre adresse IP locale (0.0.0.0 en IPv4 ou :: en IPv6).
		// 2. service (port_str.c_str()) : Le port sur lequel écouter (ex: "8080"). La fonction 
		//    attend une chaîne C standard (const char *), d'où l'utilisation de .c_str().
		// 3. hints (&hints) : L'adresse de notre structure de filtres configurée juste au-dessus. 
		//    Elle restreint les résultats (ex: uniquement TCP, accepte IPv4/IPv6).
		// 4. res (&res) : L'adresse de notre pointeur. getaddrinfo va créer la liste chaînée 
		//    et modifier ce pointeur pour qu'il cible le premier élément de la liste.
		if ((status = getaddrinfo(NULL, port_str.c_str(), &hints, &res)) != 0) {
			// Si getaddrinfo échoue, on lance une exception personnalisée
			throw GaiError("DNS/Setup Error", status);
		}

		std::cout << "Booting up server on port " << port_str << "..." << std::endl;

		// --- Boucle sur toutes les interfaces réseau trouvées ---
		for (p = res; p != NULL; p = p->ai_next) {
			void                *addr;  // Pointeur générique pour extraire l'adresse brute
			std::string         ipver;  // Stockera "IPv4" ou "IPv6" pour l'affichage
			struct sockaddr_in  *ipv4;  // Pointeur pour caster en structure IPv4
			struct sockaddr_in6 *ipv6;  // Pointeur pour caster en structure IPv6

			// Identification de la version de l'IP
			if (p->ai_family == AF_INET) {  // Si c'est une adresse IPv4
				ipv4 = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);  // Cast de l'adresse générique en structure IPv4
				addr = &(ipv4->sin_addr);   // Extraction de l'adresse IPv4 brute (sin_addr)
				ipver = "IPv4";             // Mise à jour de la variable d'affichage pour indiquer IPv4
			} else {                        // Sinon, c'est une adresse IPv6 (AF_INET6)
				ipv6 = reinterpret_cast<struct sockaddr_in6 *>(p->ai_addr); // Cast de l'adresse générique en structure IPv6
				addr = &(ipv6->sin6_addr);  // Extraction de l'adresse IPv6 brute (sin6_addr)
				ipver = "IPv6";             // Mise à jour de la variable d'affichage pour indiquer IPv6
			}

			// Conversion de l'adresse IP brute en une chaîne de caractères lisible
			// Cette fonction convertit une adresse IP binaire (format réseau, incompréhensible pour 
			// un humain) en une chaîne de caractères lisible (format présentation, ex: "127.0.0.1").
			//
			// Paramètres :
			// 1. Famille (p->ai_family) : Indique si on lit une adresse IPv4 (AF_INET) ou IPv6 (AF_INET6).
			// 2. Source (addr) : Le pointeur vers l'adresse binaire brute que l'on a extraite du 'if' précédent.
			// 3. Destination (ip_buffer) : Le tableau de caractères (buffer) où la fonction va écrire le texte.
			// 4. Taille (sizeof(ip_buffer)) : La taille maximale du buffer pour éviter un "buffer overflow".
			inet_ntop(p->ai_family, addr, ip_buffer, sizeof(ip_buffer));
			std::string ipstr(ip_buffer);
			std::cout << "Local interface found -> " << ipver << ": " << ipstr << std::endl;

			// Création du socket
			// Cette fonction demande au système d'exploitation de créer un point de terminaison 
			// (endpoint) pour la communication réseau et renvoie un "File Descriptor" (un simple 
			// numéro entier qui servira d'identifiant pour cette socket par la suite).
			//
			// Paramètres (tous fournis par la structure 'p' actuelle de notre liste getaddrinfo) :
			// 1. Domaine/Famille (p->ai_family) : Le format de l'adresse (ex: AF_INET pour IPv4, 
			//    AF_INET6 pour IPv6).
			// 2. Type (p->ai_socktype) : Le type de communication (ex: SOCK_STREAM pour du TCP, 
			//    qui garantit que les données arrivent dans l'ordre et sans perte).
			// 3. Protocole (p->ai_protocol) : Le protocole spécifique à utiliser (souvent 0 pour 
			//    laisser le système choisir le protocole par défaut lié au type, ici TCP).
			//
			// Retour :
			// Renvoie un entier positif (le File Descriptor) si le succès est total, ou -1 en cas 
			// d'échec (auquel cas la variable globale 'errno' est modifiée pour indiquer l'erreur).
			sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			// Si la création du socket échoue, on affiche une erreur et on continue avec la prochaine interface réseau disponible
			if (sockfd == -1) {
				std::cerr << "Failed to create socket: " << strerror(errno) << ". Moving to next..." << std::endl;
				continue ;
			}
			std::cout << "Socket successfully created!" << std::endl;

			// Permet de réutiliser le port immédiatement après l'arrêt du serveur (évite l'erreur "Address already in use") lors du redémarrage rapide du serveur
			// Cette fonction modifie les options de comportement par défaut du socket.
			// Ici, on l'utilise pour régler le problème du port bloqué (état TIME_WAIT). Quand on 
			// ferme le serveur, le système garde le port "réservé" pendant un moment par sécurité. 
			// SO_REUSEADDR force le système à nous laisser réutiliser ce port immédiatement.
			//
			// Paramètres :
			// 1. sockfd : Le socket que l'on vient de créer.
			// 2. Niveau (SOL_SOCKET) : Indique qu'on applique l'option au niveau général de la 
			//    socket, et non à un niveau protocole spécifique (comme le niveau TCP).
			// 3. Option (SO_REUSEADDR) : L'option spécifique que l'on veut modifier (Réutiliser l'adresse).
			// 4. Valeur (&yes) : Un pointeur vers notre variable 'yes' (qui vaut 1). Cela agit 
			//    comme un booléen "true" pour activer l'option.
			// 5. Taille (sizeof(int)) : La taille en octets de la variable pointée.
			setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

			std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;

			// --- Liaison (bind) du socket à l'IP et au port ---
			// Cette fonction associe (bind) le socket que nous venons de créer à une adresse IP 
			// et à un port spécifiques sur la machine locale. C'est ce qui permet au système 
			// d'exploitation de savoir que tout le trafic réseau arrivant sur le port 8080 
			// doit être redirigé vers notre programme.
			//
			// Paramètres :
			// 1. Socket (sockfd) : Le File Descriptor du socket
			// 2. Adresse (p->ai_addr) : Un pointeur vers la structure contenant l'IP locale et le port 
			// Cette structure a été préparée par getaddrinfo().
			// 3. Taille (p->ai_addrlen) : La taille en octets de cette structure d'adresse.
			//
			// Retour :
			// Renvoie 0 en cas de succès, ou -1 si la liaison échoue (par exemple, si le port 
			// est déjà utilisé par un autre programme en arrière-plan, ou si on essaie de se 
			// lier à un port protégé
			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				std::cerr << "Bind failed: " << strerror(errno) << " Closing socket..." << std::endl;
				// Si la liaison échoue, on ferme le socket proprement et on continue avec la prochaine interface réseau disponible
				close(sockfd);
				continue ;
			}
			std::cout << "Successfully bound to " << ipstr << " on port " << port_str << "!" << std::endl;
			break ;
		}

		// Libération de la mémoire allouée par getaddrinfo
		freeaddrinfo(res);

		// Si p est NULL, cela signifie que nous avons parcouru toute la liste des interfaces réseau possibles sans réussir à faire bind() sur aucune d'entre elles. C'est une erreur critique pour le serveur, car cela signifie que nous ne pouvons pas écouter les connexions entrantes.
		if (p == NULL) {
			throw SystemError("Fatal error: Failed to bind to any of the local interfaces");
		}

		std::cout << "Setting up the listener..." << std::endl;
		// --- Mise en écoute (listen) du socket ---
		// Cette fonction transforme le socket "active" en socket "passive". Elle indique 
		// au système d'exploitation que notre programme est désormais prêt à écouter et à 
		// accepter des connexions entrantes sur le port que l'on vient de bind().
		//
		// Paramètres :
		// 1. Socket (sockfd) : Le File Descriptor du socket principal.
		// 2. File d'attente (BACKLOG) : Définit la taille maximale de la file d'attente pour 
		//    les connexions en attente. Si 10 clients (la valeur de notre BACKLOG) essaient de 
		//    se connecter en même temps, le système les met en pause le temps qu'on s'en occupe. 
		//    Si un 11ème client arrive alors que la file est pleine, le système refusera sa 
		//    connexion (erreur ECONNREFUSED).
		//
		// Retour :
		// Renvoie 0 en cas de succès, ou -1 si la mise en écoute échoue.
		if (listen(sockfd, BACKLOG) == -1) {
			throw SystemError("Fatal error: listen() failed");
		}
		std::cout << "Server is now actively listening on port " << port_str << "! (Backlog: 10)" << std::endl;

		// EMMA : Décommenter cette partie pour faire du non-blocking et du poll, et gérer plusieurs clients en même temps
		/*
		if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
			throw SystemError("Fatal error: fcntl() failed to set listening socket to non-blocking mode");
		}
		std::cout << "Server socket is now non-blocking!" << std::endl;
		*/

		std::cout << "Entering the main server loop..." << std::endl;

		// Structure pour stocker les clients connectés (La clé est le file descriptor, la valeur est l'objet Client)
		std::map<int, Client> clients;

		// Boucle principale du serveur : accepte les connexions entrantes et gère les clients
		// Cette boucle infinie 'while (true)' maintient le serveur en vie indéfiniment. 
		// Actuellement, c'est un modèle "bloquant" : le serveur ne peut s'occuper que d'un 
		// seul client à la fois, du début à la fin de sa requête, avant de passer au suivant.
		//
		// Chaque tour de boucle représente un cycle de vie complet, divisé en 4 grandes étapes :
		// 1. Le Nettoyage (Housekeeping) : Vérification et suppression des clients inactifs (Timeouts).
		// 2. L'Acceptation : Attente et création d'une connexion avec un nouveau client.
		// 3. La Lecture (Recv) : Réception et stockage de la requête envoyée par le client.
		// 4. L'Écriture (Send) et la Déconnexion : Envoi de la réponse et fermeture de la ligne.
		while (true) {
			// =========================================================================================
			// ÉTAPE 1 : LE NETTOYAGE (HOUSEKEEPING ET TIMEOUTS)
			// =========================================================================================
			// Avant d'accepter de nouvelles connexions, le serveur inspecte la liste des clients 
			// connectés. Si un client n'a montré aucune activité (lecture ou écriture) depuis 
			// plus de MAX_TIMEOUT secondes, il est considéré comme "mort" (connexion fantôme).
			// Le serveur ferme alors sa socket et supprime ses données pour libérer de la mémoire.
			
			// Récupère l'heure actuelle sous forme de "Timestamp UNIX" (nombre de secondes depuis le 1er janvier 1970).
			// On utilise NULL car on veut juste que la fonction nous retourne la valeur, sans la stocker dans un pointeur externe.
			time_t current_time = std::time(NULL);

			// =========================================================================================
			// ITÉRATION SÉCURISÉE SUR LA MAP DES CLIENTS
			// =========================================================================================
			// Attention : La boucle 'for' ci-dessous n'a pas de '++it' dans sa déclaration !
			// C'est volontaire. Supprimer un élément d'une std::map pendant qu'on la parcourt détruit 
			// la case mémoire sur laquelle on se trouve (invalidation de l'itérateur). Si on faisait 
			// un '++it' classique juste après un 'erase', le programme crasherait.
			for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ) {
				// it->first correspond à la clé (le FD de la socket).
				// it->second correspond à la valeur (l'objet Client).
				// On utilise une référence '&' pour manipuler le vrai client stocké dans la map, et non une copie.
				Client &client = it->second;

				// std::difftime calcule la différence en secondes entre l'heure actuelle et le dernier signe de vie du client.
				if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
					std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
					// On met à jour la machine à états pour indiquer la fin de vie du client.
					client.setState(Client::DISCONNECTED);
					// On ferme proprement la connexion réseau au niveau du système d'exploitation.
					close(client.getSocketFd());
					// Le curseur 'it' avance vers le client SUIVANT pour se mettre en sécurité, 
					// mais il donne à la fonction erase() l'ANCIENNE position pour qu'elle soit supprimée.
					// Ainsi, le client inactif est effacé de la mémoire sans faire planter la boucle.
					clients.erase(it++);
				}
				else {
					// Si le client est toujours actif (pas de timeout), on avance simplement le curseur 
					// manuellement vers le client suivant pour continuer notre vérification.
					++it;
				}
			}
			
			// =========================================================================================
			// ÉTAPE 2 : L'ACCEPTATION D'UN NOUVEAU CLIENT
			// =========================================================================================
			// La fonction accept() met le programme "en pause" (elle bloque) jusqu'à ce qu'un 
			// utilisateur tente de se connecter sur le port 8080.
			// Une fois qu'un client arrive, le serveur lui attribue un nouveau "File Descriptor" 
			// dédié (client_fd), et crée son objet Client correspondant.


			// EMMA : Code temporaire (bloquant) pour tester la logique avant d'implémenter poll()
			// Tant que poll() n'est pas en place et que sockfd n'est pas non-bloquant, le programme 
			// va s'arrêter net sur la ligne accept() et attendre indéfiniment qu'un client arrive.
			std::cout << "Waiting for incoming connections... (Program is blocked here)" << std::endl;

			// 'sockaddr_storage' est une structure "fourre-tout" très pratique. Elle est conçue pour 
			// être suffisamment grande pour contenir une adresse réseau, que le client utilise du 
			// vieux IPv4 ou du nouvel IPv6.
			struct sockaddr_storage client_addr;

			// =========================================================================================
			// EXPLICATION DE socklen_t ET addr_size (Le mécanisme Value-Result) :
			// =========================================================================================
			// 1. Pourquoi 'socklen_t' et pas 'int' ? 
			// C'est un type défini par le système spécifiquement pour la taille des adresses réseau. 
			// L'utiliser garantit que votre code ne plantera pas si vous le compilez sur une machine 
			// 64 bits ou 32 bits (la taille d'un entier peut varier selon les systèmes).
			//
			// 2. Le double rôle de 'addr_size' :
			// Cette variable sert à la fois de consigne et de compte-rendu pour la fonction accept().
			// - Avant accept() : On la remplit avec la taille totale de 'client_addr' (sizeof). 
			//   Cela dit au système : "Attention, tu as le droit d'écrire au maximum X octets en mémoire."
			// - Pendant accept() : On donne son adresse mémoire (&addr_size). Le système lit notre 
			//   limite, écrit l'adresse IP du client, puis MODIFIE notre variable 'addr_size' pour 
			//   y inscrire la taille *réelle* qu'il a finalement utilisée !
			// =========================================================================================
			socklen_t               addr_size;

			// 'client_fd' sera le descripteur du NOUVEAU socket créé spécifiquement pour ce client.
			int                     client_fd;

			// accept() a besoin de savoir combien d'espace mémoire on lui a réservé au maximum
			addr_size = sizeof(client_addr);

			// Cette fonction prend la première connexion en attente dans la file (le BACKLOG) et 
			// crée un NOUVEAU socket dédié uniquement à la communication avec ce client précis.
			// Le socket principal d'écoute (sockfd) reste intact en arrière-plan !
			//
			// Paramètres :
			// 1. sockfd : Notre socket principal d'écoute (celui créée tout en haut du programme).
			// 2. Adresse (&client_addr) : Un pointeur vers notre structure vide. Le système d'exploitation 
			//    va y écrire l'adresse IP et le port du client qui vient de se connecter. L'API 
			//    C attend un 'struct sockaddr *', d'où le reinterpret_cast.
			// 3. Taille (&addr_size) : Un pointeur vers la taille de notre structure. Le système la 
			//    mettra à jour avec la taille réelle des données qu'il a écrites.
			//
			// Retour :
			// Renvoie un entier positif (le nouveau File Descriptor) en cas de succès, ou -1 en cas d'échec.
			// =========================================================================================
			client_fd = accept(sockfd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);

			// =====================================================================================
				// GESTION D'ERREUR NON-FATALE (ROBUSTESSE DU SERVEUR)
				// =====================================================================================
				// Si accept() renvoie -1, la connexion avec CE client précis a échoué (ex: il a fermé 
				// son navigateur à la dernière milliseconde, ou le réseau a sauté).
				// Règle d'or : Un serveur ne doit jamais crasher à cause de l'erreur d'un seul client !
				// On utilise donc 'continue' au lieu d'une exception ('throw'). Cela permet d'ignorer 
				// le reste du code pour ce client fantôme, et de remonter immédiatement au début de la 
				// boucle while(true) pour attendre le client suivant.
				// =====================================================================================
				if (client_fd == -1) {
				std::cerr << "Error: accept() failed: " << strerror(errno) << std::endl;
				continue ;
			}

			// =========================================================================================
			// CRÉATION ET INITIALISATION DU CLIENT (L'OBJET)
			// =========================================================================================
			// Une fois la connexion réseau établie via accept(), on doit créer un espace en mémoire 
			// pour suivre l'évolution de ce client. C'est le rôle de l'objet 'Client'.
			// 
			// Dès que cet objet est instancié, il s'initialise automatiquement ainsi :
			// 1. _socket_fd      : Prend la valeur du 'client_fd' (ou -1 par défaut) pour lier 
			//                      logiquement l'objet à la connexion physique.
			// 2. _state          : Est initialisé à READING_REQUEST. Le client vient d'arriver, 
			//                      on attend donc qu'il nous envoie sa requête HTTP.
			// 3. _last_activity  : Est initialisé à time(NULL). time() lit l'horloge système et 
			//                      renvoie le "Timestamp UNIX" (le nombre de secondes depuis 1970). 
			//                      On lui passe NULL pour lui dire de nous retourner cette valeur 
			//                      directement au lieu de la stocker via un pointeur. Le chronomètre 
			//                      de sécurité (Timeout) démarre donc à la seconde exacte de la connexion
			// =========================================================================================
			Client  client(client_fd, client_addr);
			
			// =========================================================================================
			// L'INTÉGRATION DU CLIENT DANS LA LOGIQUE DU SERVEUR
			// =========================================================================================
			// Maintenant que la connexion réseau est établie et que l'objet local 'client' est créé, 
			// nous devons le préparer et l'enregistrer officiellement dans les registres du serveur.

			// 1. Démarrage du chronomètre (Sécurité)
			// On s'assure que le marqueur de dernière activité est réglé à la seconde exacte de 
			// l'acceptation. Si ce client se connecte mais reste complètement muet pendant 10 
			// secondes (MAX_TIMEOUT), l'Étape 1 (le nettoyeur au début de la boucle) le détectera 
			// et coupera la connexion pour libérer la place.
			client.updateLastActivity();

			// 2. Initialisation de la Machine à États (State Machine)
			// Dans le protocole HTTP, c'est TOUJOURS le client qui doit parler en premier 
			// (pour demander une page web ou envoyer un formulaire). L'état initial est donc 
			// logiquement réglé sur READING_REQUEST. Le serveur sait ainsi que son prochain rôle 
			// avec ce client sera d'écouter (via recv).
			client.setState(Client::READING_REQUEST);

			// 3. Archivage dans le dictionnaire principal (La Map)
			// C'est l'étape la plus cruciale ! L'objet 'client' qu'on vient de créer et de modifier 
			// est une variable locale temporaire. Pour que le serveur s'en souvienne aux prochains 
			// tours de boucle (et quand poll() sera là), on le stocke dans notre grand conteneur 
			// 'clients', en utilisant son numéro de socket (client_fd) comme clé d'accès (ID unique).
			clients[client_fd] = client;

			// =========================================================================================
			// AFFICHAGE DES LOGS DE CONNEXION (ACCESS LOGS)
			// =========================================================================================
			// Maintenant que l'objet Client a été créé et rangé dans la map, on affiche les logs
			std::cout << "CONNECTION ACCEPTED!" << std::endl;
			std::cout << "Client IP: " << clients[client_fd].getIp() << std::endl;
			std::cout << "Communication is now open on new socket: " << clients[client_fd].getSocketFd() << std::endl;
			std::cout << "Listening socket " << sockfd << " is still active in the background." << std::endl;

			// =========================================================================================
			// ÉTAPE 3 : LA LECTURE DE LA REQUÊTE (RECV)
			// =========================================================================================
			// Maintenant que la ligne est ouverte, le serveur écoute ce que le client a à dire.
			// La fonction recv() bloque à nouveau le programme jusqu'à ce que le client envoie 
			// des données (la requête HTTP). 
			// Si le client ferme la page web avant d'envoyer sa requête, recv() renvoie 0 (déconnexion).
			// Si la lecture réussit, on met à jour le chronomètre d'activité et on passe l'état 
			// du client en PROCESSING (requête HTTP prête à être analysé).

			// 1. Préparation de l'espace de réception (Le Buffer)
			// On crée un petit bac à sable de 1024 octets pour stocker temporairement les données reçues.
			char buffer[1024];
			// On nettoie la mémoire pour s'assurer qu'il n'y a pas de garbage memory
			memset(buffer, 0, sizeof(buffer));

			// 2. Appel à recv()
			// recv() va copier les données depuis la carte réseau vers notre 'buffer'.
			//
			// Paramètres :
			// - client_fd : L'ID du socket client
			// - buffer : Où stocker les données.
			// - sizeof(buffer) - 1 : On garde un octet pour le '\0' final (sécurité C-string).
			// - 0 : Pas de flags particuliers ici.
			//
			// Valeur de retour (bytes_received) :
			// > 0 : Nombre d'octets reçus.
			// = 0 : Le client a raccroché proprement (FIN).
			// < 0 : Erreur réseau.
			ssize_t bytes_received = recv(client.getSocketFd(), buffer, sizeof(buffer) - 1, 0);

			// 3. Analyse du résultat de la lecture
			if (bytes_received < 0) {
				// Cas d'erreur : Problème matériel ou réseau brutal.
				std::cerr << "Error reading from socket." << std::endl;
			} else if (bytes_received == 0) {
				// Cas de déconnexion : Le client a fermé l'onglet ou la connexion.
				std::cout << "Client unexpectedly closed the connection." << std::endl;
				// Nettoyage complet
				clients[client_fd].setState(Client::DISCONNECTED);
				close(client.getSocketFd());
				clients.erase(client.getSocketFd());
			} else {

				// =========================================================================================
				// ÉTAPE 4 : L'ANALYSE, L'ENVOI DE LA RÉPONSE (SEND) ET LA DÉCONNEXION
				// =========================================================================================
				// Le serveur a reçu la demande. Il prépare la réponse (Parsing HTTP) puis utilise 
				// la fonction send() pour expédier le résultat sur le réseau (état WRITING_RESPONSE).
				// Dans ce modèle basique, dès que la réponse est envoyée, le serveur raccroche 
				// immédiatement (close), supprime le client (erase), et retourne au début de la boucle 
				// pour attendre le client suivant.'

				// 1. Récupération de la référence (Le vrai objet dans le registre)
				// On utilise une référence (&) pour ne pas copier l'objet. Toute modification sur 
				// 'current_client' sera immédiatement enregistrée dans la map 'clients'.
				Client &current_client = clients[client_fd];

				// 2. Mise à jour du "Signe de Vie" (Sécurité Timeout)
				// Le client vient d'envoyer des données, il est donc actif. On réinitialise son 
				// chronomètre pour qu'il ne soit pas expulsé par le nettoyeur au début du prochain tour.
				current_client.updateLastActivity();

				std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM CLIENT ---" << std::endl;
				
				// 3. Conversion des données brutes en chaîne C++
				// Le buffer est un tableau de char (C-style). On le transforme en std::string 
				// pour faciliter le futur parsing
				std::string received_data(buffer, bytes_received);
				std::cout << received_data << std::endl;
				std::cout << "--------------------------------------" << std::endl;

				// 4. Changement d'État : Passage au mode "Réflexion"
				// On a fini de lire (READING_REQUEST). On passe maintenant à l'analyse 
				// de ce que le client veut (PROCESSING).
				current_client.setState(Client::PROCESSING);
				
				// =========================================================
				// ROMANE : IMPLÉMENTER LE PARSING HTTP ICI
				// 1. Stocker le contenu de 'buffer' dans client._read_buffer
				//    (ex: client.appendReadBuffer(buffer, bytes_received))
				// 2. Vérifier si on a reçu une requête HTTP complète (présence de "\r\n\r\n")
				// 3. Analyser la requête (GET, POST, URI, Headers)
				// 4. Générer la réponse HTTP correspondante (200 OK, 404 Not Found, etc.)
				//    et la stocker dans un buffer d'écriture (ex: client._write_buffer)
				// 5. Modifier le client.state pour passer en WRITING_RESPONSE 
				//    (nécessaire une fois que poll sera en place)
				// =========================================================

				// --- SEND (Sending the response) ---
				// ROMANE : Temporairement, on envoie une réponse statique pour tester.
				// À terme, cette partie devra envoyer le contenu de client._write_buffer
				std::string response = "Good talking to you!\n";

				// 2. CHANGEMENT D'ÉTAT
				// On informe le système que nous avons fini de réfléchir et que nous sommes 
				// prêts à écrire sur le réseau.
				current_client.setState(Client::WRITING_RESPONSE);

				// 3. EXPÉDITION DE LA RÉPONSE (Le rôle de send)
				// Contrairement à ce qu'on pourrait penser, send() ne garantit pas que les 
				// données sont arrivées chez le client. Son rôle est de copier les données 
				// de notre programme vers le "Buffer d'Envoi" du noyau (Kernel).
				//
				// Paramètres :
				// - client_fd : Le canal sur lequel on écrit.
				// - response.c_str() : Pointeur vers le début de notre chaîne de caractères.
				// - response.size() : Le nombre total d'octets que l'on souhaite envoyer.
				// - 0 : Pas de flags spécifiques.
				//
				// Comportement crucial (Blocage vs Non-Blocage) :
				// - ACTUELLEMENT (Mode Bloquant) : Si le buffer du système est plein (connexion 
				//   lente), send() va arrêter net le programme et attendre qu'il y ait de la 
				//   place avant de continuer. Il renvoie généralement la totalité de la taille.
				// - BIENTÔT (Mode Non-Bloquant avec epoll) : send() prendra ce qu'il peut 
				//   (peut-être seulement 50% des données) et rendra la main immédiatement. 
				//   C'est là que l'on devra gérer un "Write Buffer" pour envoyer le reste plus tard.
				//
				// Retour (bytes_sent) : Le nombre exact d'octets que le système a accepté de prendre en charge.
				ssize_t bytes_sent = send(client_fd, response.c_str(), response.size(), 0);

				if (bytes_sent < 0) {
					std::cerr << "Error: sendind response to " << client_fd << std::endl;
				} else {
					std::cout << "Successfully sent " << bytes_sent << " bytes back to the client " << client_fd << std::endl;
					// Même si on a envoyé une réponse, le client peut rester actif (ex: navigateur qui attend une image). On met donc à jour son "Signe de Vie" pour éviter un timeout prématuré.
					clients[client_fd].updateLastActivity();
				}

				// =========================================================================================
				// FIN DE CYCLE : DÉCONNEXION DU CLIENT (Mode temporaire : HTTP/1.0)
				// =========================================================================================
				// Actuellement, le serveur raccroche immédiatement après avoir envoyé sa réponse unique.
				// C'est le comportement basique de l'ancien protocole HTTP/1.0 (Une requête = Une connexion).
				// 
				//
				// ⚠️ NOTE POUR L'INTÉGRATION DE POLL() ET HTTP/1.1 (KEEP-ALIVE) :
				// Plus tard, on ne voudra PLUS déconnecter le client ici ! À la place des 3 lignes 
				// ci-dessous, on retirera le close() et le erase(), et on remplacera simplement :
				//      clients[client_fd].setState(Client::DISCONNECTED); 
				// par :
				//      clients[client_fd].setState(Client::READING_REQUEST);
				// Ainsi, on gardera la ligne ouverte au cas où le navigateur demanderait une image 
				// ou du CSS dans la foulée. Ce sera le Timeout (Étape 1 tout en haut) ou une 
				// fermeture volontaire du navigateur (recv() == 0) qui s'occupera du vrai nettoyage.
				// =========================================================================================

				// 1. Machine à États : On indique formellement que ce client a terminé son cycle.
				clients[client_fd].setState(Client::DISCONNECTED); 
				
				// 2. Fermeture physique (OS) : On libère le descripteur de fichier réseau.
				close(client_fd);
				
				// 3. Nettoyage de la mémoire (RAM) : On supprime le client 
				// de notre map. S'il veut nous reparler, il devra refaire un processus complet 
				// de connexion (accept) depuis le début.
				clients.erase(client_fd);
			}
		}
	}
	// =========================================================================================
	// GESTION DES ERREURS FATALES (FILET DE SÉCURITÉ)
	// =========================================================================================
	catch (const std::exception &e) {
		// Si une exception non gérée remonte jusqu'ici (ex: plus de mémoire, erreur système 
		// critique), on attrape le message d'erreur pour ne pas que le serveur disparaisse 
		// sans laisser de traces.
		std::cerr << "Fatal error: " << e.what() << std::endl;
		// On vérifie si le socket d'écoute principal est encore ouvert.
		// Si oui, on la ferme impérativement pour libérer le port 8080.
		// Sans cela, le port resterait "bloqué" par l'OS et vous ne pourriez pas 
		// redémarrer le serveur immédiatement (Erreur : Address already in use).
		if (sockfd != -1) {
			close(sockfd);
		}
		return (1);
	}
	// =========================================================================================
	// FERMETURE PROPRE (ARRÊT NORMAL)
	// =========================================================================================
	// Si le programme sort un jour de sa boucle while(true) (par exemple via un signal 
	// d'arrêt comme Ctrl+C si vous le gérez), on arrive ici.
	std::cout << "Shutting down the server (sockfd)." << std::endl;
	// On libère proprement le socket principal du serveur.
	if (sockfd != -1) {
		close(sockfd);
	}
	return (0);
}
