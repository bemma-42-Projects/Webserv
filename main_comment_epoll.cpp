#define _POSIX_C_SOURCE 200112L
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <map>
#include <ctime>
#include <iostream>
#include <sys/epoll.h>
#include <errno.h>
#include <cstring>
#include "Client.hpp"
#include "SystemError.hpp"
#include "GaiError.hpp"
#define PORT "8080"
#define BACKLOG 10
#define MAX_TIMEOUT 10
#define MAX_EVENTS 100

// =========================================================================
// FONCTION UTILITAIRE : PASSAGE EN MODE NON-BLOQUANT
// =========================================================================
// Cette fonction modifie le comportement d'un file descriptor (ici notre socket) 
// pour qu'il ne bloque plus l'exécution du programme lors des appels réseau 
// (accept, recv, send). C'est indispensable pour l'utilisation de poll().
void set_nonblocking(int fd) {
    // 1. LECTURE DES PARAMÈTRES ACTUELS
    // La fonction fcntl (File Control) avec la commande F_GETFL (Get File Status Flags) 
    // interroge le système d'exploitation pour récupérer la configuration actuelle du socket.
    int flags = fcntl(fd, F_GETFL, 0);
    // Sécurité : on vérifie que le fd est valide et que le système a bien répondu.
    if (flags == -1) {
        std::cerr << "Error: fcntl(F_GETFL) failed." << std::endl;
		return;
    }
    // 2. MODIFICATION SÉCURISÉE ET APPLICATION
    // On utilise F_SETFL (Set File Status Flags) pour imposer la nouvelle configuration.
    // L'astuce cruciale est ici : 'flags | O_NONBLOCK'. 
    // L'opérateur binaire '|' (OU inclusif) permet d'ajouter le statut NON-BLOQUANT 
    // à la configuration existante ('flags') SANS écraser d'éventuels autres paramètres.
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Error: fcntl(F_SETFL) failed." << std::endl;
	}
}
/*
int main(void)
{
    struct addrinfo         hints;                          
    struct addrinfo         *res;                           
    struct addrinfo         *p;                             
    int                     status;                         
    char                    ip_buffer[INET6_ADDRSTRLEN];    
    int                     sockfd = -1;                    
    int                     yes = 1;                        
    const std::string       port_str = PORT; 
    try {
        memset(&hints, 0, sizeof hints);   
        hints.ai_family = AF_UNSPEC;            
        hints.ai_socktype = SOCK_STREAM;        
        hints.ai_flags = AI_PASSIVE;            
        if ((status = getaddrinfo(NULL, port_str.c_str(), &hints, &res)) != 0) {
            throw GaiError("DNS/Setup Error", status);
        }
        std::cout << "Booting up server on port " << port_str << "..." << std::endl;
        for (p = res; p != NULL; p = p->ai_next) {
            void                *addr;  
            std::string         ipver;  
            struct sockaddr_in  *ipv4;  
            struct sockaddr_in6 *ipv6;  
            if (p->ai_family == AF_INET) {  
                ipv4 = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);  
                addr = &(ipv4->sin_addr);   
                ipver = "IPv4";             
            } else {                        
                ipv6 = reinterpret_cast<struct sockaddr_in6 *>(p->ai_addr); 
                addr = &(ipv6->sin6_addr);  
                ipver = "IPv6";             
            }
            inet_ntop(p->ai_family, addr, ip_buffer, sizeof(ip_buffer));
            std::string ipstr(ip_buffer);
            std::cout << "Local interface found -> " << ipver << ": " << ipstr << std::endl;
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd == -1) {
                std::cerr << "Failed to create socket: " << strerror(errno) << ". Moving to next..." << std::endl;
                continue ;
            }
            std::cout << "Socket successfully created!" << std::endl;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
            std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                std::cerr << "Bind failed: " << strerror(errno) << " Closing socket..." << std::endl;
                close(sockfd);
                continue ;
            }
            std::cout << "Successfully bound to " << ipstr << " on port " << port_str << "!" << std::endl;
            break ;
        }
        freeaddrinfo(res);
        if (p == NULL) {
            throw SystemError("Fatal error: Failed to bind to any of the local interfaces");
        }
        std::cout << "Setting up the listener..." << std::endl;
        if (listen(sockfd, BACKLOG) == -1) {
            throw SystemError("Fatal error: listen() failed");
        }

        std::cout << "Server is now actively listening on port " << port_str << "! (Backlog: " << BACKLOG << ")" << std::endl;
        
        // Passage du socket d'écoute en mode non-bloquant (O_NONBLOCK) via fcntl.
        set_nonblocking(sockfd);

        // CRÉATION DE L'INSTANCE EPOLL
        // Appel système demandant au noyau Linux d'allouer une nouvelle instance epoll.
        // MAX_EVENTS est une indication
        // du nombre de descripteurs à surveiller.
        // Retourne un descripteur de fichier (epoll_fd) pointant vers cette instance 
        // dans l'espace noyau.
        int epoll_fd = epoll_create(MAX_EVENTS);
        
        // GESTION D'ERREUR SYSTÈME
        // Retourne -1 et set errno si l'allocation de l'instance epoll échoue.
        if (epoll_fd == -1) {
            throw std::runtime_error("Fatal error: epoll_create() failed");
        }

        // CONFIGURATION DE L'ÉVÉNEMENT POUR LE SOCKET D'ÉCOUTE
        // Initialisation de la structure epoll_event requise par epoll_ctl.
        struct epoll_event ev;

        // DÉFINITION DU FLAG DE SURVEILLANCE ET ASSOCIATION DU FD
        // EPOLLIN : Instruction pour le noyau de surveiller la disponibilité de données en lecture.
        // Sur un "listen socket" (sockfd), la présence de données en lecture (EPOLLIN) 
        // indique qu'une nouvelle connexion entrante est présente dans la file d'attente 
        // (backlog) et est prête à être extraite via accept().
        // ev.data.fd permet d'identifier la source de l'événement lors du retour de epoll_wait().
        ev.events = EPOLLIN;

        // ASSOCIATION DU DESCRIPTEUR DANS L'UNION DE DONNÉES UTILISATEUR (epoll_data_t)
        // L'union 'ev.data' sert de payload renvoyé par le noyau lors d'un événement.
        // En y stockant 'sockfd', on s'assure que lors du déclenchement de epoll_wait(), 
        // le système nous retournera ce descripteur exact, permettant ainsi d'identifier 
        // de manière univoque la source (le socket d'écoute) ayant généré l'interruption (EPOLLIN).
        ev.data.fd = sockfd;

        // ENREGISTREMENT DU DESCRIPTEUR DANS LA LISTE D'INTÉRÊT (INTEREST LIST) DU NOYAU
        // L'appel système epoll_ctl modifie l'instance epoll pointée par 'epoll_fd'.
        // - L'opération EPOLL_CTL_ADD instruit le noyau d'insérer le descripteur cible 'sockfd'
        //   dans l'arbre rouge-noir (Red-Black tree) de l'instance epoll.
        // - Le pointeur '&ev' transmet les paramètres de surveillance associés (ici, EPOLLIN).
        // Si l'opération échoue (ex: descripteur invalide, limite de mémoire du noyau atteinte), 
        // l'appel renvoie -1 et lève une exception std::runtime_error pour stopper l'exécution.
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
            throw std::runtime_error("Fatal error: epoll_ctl() failed on sockfd");
        }

	    // ALLOCATION DU BUFFER UTILISATEUR POUR LA RÉCEPTION DES ÉVÉNEMENTS
        // Déclaration d'un tableau de structures 'epoll_event' alloué dans l'espace 
        // utilisateur (user space), sur la stack.
        // Ce tableau agit comme un buffer de destination obligatoire pour l'appel 
        // système epoll_wait() qui sera exécuté dans la boucle principale.
        // Lors du retour de epoll_wait(), le noyau Linux copiera les descripteurs 
        // passés à l'état "prêt" (depuis sa ready list interne) directement dans ce tableau.
        // La constante MAX_EVENTS définit la limite supérieure stricte du nombre d'événements 
        // que le noyau est autorisé à copier en un seul appel (batching), empêchant ainsi 
        // tout débordement de mémoire (buffer overflow) tout en optimisant les changements 
        // de contexte (context switches) entre le noyau et l'application.
        struct epoll_event events[MAX_EVENTS];

        std::cout << "Entering the main server loop..." << std::endl;
        std::map<int, Client> clients;

        while (1) {
            time_t current_time = std::time(NULL);
            for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ) {
                Client &client = it->second;
                if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
                    std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
                    client.setState(Client::DISCONNECTED);

                    // DÉSINREGISTREMENT EXPLICITE DU DESCRIPTEUR DANS L'INSTANCE EPOLL (EPOLL_CTL_DEL)
                    // L'appel système epoll_ctl est invoqué avec la macro d'opération EPOLL_CTL_DEL.
                    // Cette commande instruit le noyau Linux de purger immédiatement le descripteur 
                    // de fichier du client (client.getSocketFd()) de la liste d'intérêt (interest list) 
                    // maintenue dans l'arbre rouge-noir (Red-Black tree) de l'instance 'epoll_fd'.
                    //
                    // Le dernier paramètre est fixé à NULL : lors d'une opération de suppression (DEL), 
                    // le noyau n'attend aucune nouvelle instruction de surveillance, il n'est donc 
                    // pas nécessaire de lui transmettre l'adresse d'une structure 'epoll_event'.
                    //
                    // PRATIQUE DÉFENSIVE D'ARCHITECTURE SYSTÈME :
                    // Bien que l'appel système close() qui suit retire automatiquement le descripteur 
                    // de l'instance epoll (si son compteur de références système tombe à zéro), 
                    // invoquer explicitement EPOLL_CTL_DEL avant le close() est une norme de robustesse.
                    // Cela garantit la cessation immédiate de la surveillance de l'I/O par le noyau et 
                    // prévient l'apparition d'événements fantômes (phantom events) dans le cas où le 
                    // file descriptor aurait été dupliqué dans l'espace noyau (via dup() ou fork()).
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.getSocketFd(), NULL);
                    close(client.getSocketFd());
                    clients.erase(it++);
                }
                else {
                    ++it;
                }
            }

            // ATTENTE SYNCHRONE DES ÉVÉNEMENTS (MULTIPLEXAGE D'ENTRÉES/SORTIES)
            // L'appel système epoll_wait suspend l'exécution du thread courant (context switch) 
            // jusqu'à ce qu'au moins un événement I/O survienne sur les descripteurs surveillés, 
            // ou jusqu'à l'expiration du délai d'attente (timeout).
            // Le noyau peuple le tableau 'events' alloué dans l'espace utilisateur et retourne 
            // le nombre de descripteurs prêts (n_events).
            //
            // GESTION DU TIMEOUT (PARAMÈTRE CRITIQUE) :
            // La valeur '1000' (en millisecondes) force le noyau à réveiller le thread au maximum 
            // toutes les secondes, même en l'absence de trafic réseau. 
            // L'utilisation de '-1' (blocage infini) est proscrite ici : elle empêcherait 
            // l'exécution de la routine de nettoyage (housekeeping) située en début de boucle 
            // qui vérifie l'inactivité des clients (std::difftime).
            
            int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

            // GESTION D'ERREUR SYSTÈME ET INTERRUPTIONS
            if (n_events == -1) {
                // Le noyau peut interrompre un appel système bloquant (comme epoll_wait) 
                // pour traiter un signal asynchrone (ex: SIGWINCH, SIGCHLD).
                // Dans ce cas, errno est défini sur EINTR (Interrupted system call).
                // Ce n'est pas une erreur fatale : on instruit le programme de relancer la boucle.
                if (errno == EINTR) {
                    continue;
                }
                // Pour toute autre erreur critique (ex: EBADF si epoll_fd est corrompu, 
                // ou EFAULT si le pointeur 'events' est invalide), on lève une exception 
                // en y concaténant le message d'erreur du noyau (via std::strerror) 
                // pour un traçage précis avant l'arrêt du processus.
                throw std::runtime_error(std::string("Fatal error: epoll_wait() failed: ") + std::strerror(errno));
            }

            // ITÉRATION SUR LA READY LIST DU NOYAU
            // Le tableau 'events' a été populé par epoll_wait avec 'n_events' descripteurs 
            // ayant changé d'état. On itère séquentiellement pour dispatcher le traitement.
            for (int i = 0; i < n_events; i++) {
                // On récupère le numéro du socket (le "File Descriptor") qui vient de s'activer.
                // C'est avec ce socket qu'on va devoir interagir (lire ou écrire) juste en dessous.
                int active_fd = events[i].data.fd;

                // -----------------------------------------------------------------
                // CAS 0 : GESTION DES ERREURS FATALES (EPOLLERR / EPOLLHUP)
                // -----------------------------------------------------------------
                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    std::cerr << "Epoll error or hang up on socket " << active_fd << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                    close(active_fd);
                    clients.erase(active_fd);
                    continue; // On passe direct au client suivant
                }

                // -----------------------------------------------------------------
                // CAS 1 : NOUVELLE CONNEXION ENTRANTE (Nouveau visiteur)
                // -----------------------------------------------------------------
                // Si le socket qui vient de s'activer est notre socket principal ('sockfd'),
                // cela veut dire qu'un nouveau client (navigateur web, curl...) essaie 
                // de se connecter à notre serveur. 
                // On doit utiliser accept() pour l'accueillir et lui créer un socket dédié.
                if (active_fd == sockfd) {
                    // 1. PRÉPARATION
                    // On prépare une structure suffisamment grande (sockaddr_storage) 
                    // pour stocker l'adresse IP (IPv4 ou IPv6) et le port du nouveau client.
                    struct sockaddr_storage client_addr;
                    socklen_t addr_size;

                    int client_fd;

                    addr_size = sizeof(client_addr);

                    // 2. ACCEPTATION DE LA CONNEXION
                    // accept() valide la connexion du visiteur et lui crée un nouveau socket 
                    // (client_fd) dédié uniquement à ses futurs échanges (requêtes/réponses HTTP).
                    // Comme sockfd est non-bloquant, cette fonction n'attend pas et retourne tout de suite.
                    client_fd = accept(sockfd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
                    
                    // GESTION D'ERREUR : ÉCHEC DE CONNEXION DU VISITEUR
                    // Si accept() renvoie -1, cela signifie qu'on n'a pas pu créer 
                    // le socket pour ce client spécifique (ex: la connexion a été coupée en cours de route).
                    if (client_fd == -1) {
                        // Le 'continue' permet d'abandonner uniquement ce visiteur défectueux 
                        // et de repasser au début de la boucle pour s'occuper des autres.
                        std::cerr << "Error: accept() failed: " << std::strerror(errno) << std::endl;
                        continue ;
                    }

                    // 3. SÉCURITÉ DU SERVEUR (CRITIQUE)
                    // On passe immédiatement ce NOUVEAU socket client en mode non-bloquant.
                    // C'est vital : si on ne le fait pas, le premier appel à recv() pour lire 
                    // sa requête HTTP pourrait bloquer complètement la boucle infinie du serveur !
                    set_nonblocking(client_fd);

                    Client  client(client_fd, client_addr);
                    client.updateLastActivity();
                    client.setState(Client::READING_REQUEST);
                    clients[client_fd] = client;

                    // 1. PRÉPARATION DE LA SURVEILLANCE
                    // On crée une nouvelle structure d'événement dédiée spécifiquement à ce nouveau visiteur.
                    struct epoll_event client_ev;

                    // 2. LE TYPE D'ACTION ATTENDUE
                    // EPOLLIN signifie qu'on demande à epoll de nous alerter dès qu'il y a des données "à lire".
                    // Pour notre webserv, cela correspond au moment où le client va nous envoyer 
                    // sa requête HTTP
                    client_ev.events = EPOLLIN;

                    // 3. L'ASSOCIATION AU BON SOCKET
                    // On attache le socket de ce visiteur (client_fd) à l'événement.
                    // Ainsi, quand le client enverra sa requête, epoll_wait (plus haut dans le code) 
                    // nous réveillera en nous redonnant ce numéro exact, et on saura exactement qui nous parle.
                    client_ev.data.fd = client_fd;
                    
                    // 4. L'ENREGISTREMENT OFFICIEL
                    // On demande à l'instance epoll d'ajouter (EPOLL_CTL_ADD) ce nouveau socket client (client_fd) 
                    // à sa liste de surveillance, en appliquant les règles qu'on vient de définir (client_ev).
                    // Désormais, le serveur sera alerté dès que ce visiteur enverra sa requête HTTP.
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
                        // GESTION D'ERREUR : NETTOYAGE IMMÉDIAT
                        // Si epoll refuse de surveiller ce client (ex: manque de ressources système),
                        // on affiche l'erreur proprement.
                        std::cerr << "Error: epoll_ctl(ADD) failed on client_fd " << client_fd << ": " << std::strerror(errno) << std::endl;
                        // Il est crucial de fermer le socket pour ne pas épuiser les ressources du serveur (fuite de fd).
                        close(client_fd);
                        // On le retire de notre registre (map) pour éviter un crash plus tard.
                        clients.erase(client_fd);
                        // On abandonne ce client et on passe au suivant.
                        continue;
                    }

                    std::cout << "CONNECTION ACCEPTED!" << std::endl;
                    std::cout << "Client IP: " << clients[client_fd].getIp() << std::endl;
                    std::cout << "Communication is now open on new socket: " << client_fd << std::endl;
                    std::cout << "Listening socket " << sockfd << " is still active in the background." << std::endl;
                }
                // -----------------------------------------------------------------
                // CAS 2 : LE CLIENT EXISTANT NOUS ENVOIE SA REQUÊTE HTTP
                // -----------------------------------------------------------------
                // Si l'événement est de type EPOLLIN (données à lire) et que ce n'est PAS 
                // le socket d'écoute, c'est qu'un visiteur connecté nous parle.
                else if (events[i].events & EPOLLIN) {
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));

                    // 2. LECTURE DES DONNÉES SUR LE RÉSEAU
                    // On lit ce que le client a envoyé via son socket dédié (active_fd).
                    ssize_t bytes_received;
                    bytes_received = recv(active_fd, buffer, sizeof(buffer) - 1, 0);

                    // 3. VÉRIFICATION DE L'ÉTAT DE LA CONNEXION
                    if (bytes_received <= 0) {
                        // CAS 2A : DÉCONNEXION PROPRE
                        // Le client a fermé la connexion de lui-même.
                        if (bytes_received == 0) {
                            std::cout << "Client on socket " << active_fd << " closed the connection." << std::endl;
                        }
                        // CAS 2B : ERREUR RÉSEAU
                        // recv() a retourné -1 (ex: perte de connexion Wi-Fi du client, timeout système...).
                        else {
                            std::cerr << "Error: recv() failed on socket " << active_fd << ": " << std::strerror(errno) << std::endl;
                        }
                        // 4. NETTOYAGE COMPLET DU CLIENT
                        // A. On dit à epoll d'arrêter de surveiller ce socket.
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                        // B. On ferme physiquement la connexion au niveau du système d'exploitation.
                        close(active_fd);
                        // C. On supprime le client de notre registre mémoire (la map) pour éviter les fuites.
                        clients.erase(active_fd);
                        // On passe à l'événement suivant dans la boucle for.
                        continue;
                    // -----------------------------------------------------------------
                    // CAS 2C : LECTURE RÉUSSIE (Le client nous parle)
                    // -----------------------------------------------------------------
                    } else {
                        // 1. RÉCUPÉRATION DU CLIENT
                        // On récupère une référence (&) vers notre client dans la map 
                        // pour pouvoir modifier ses données directement.
                        Client &current_client = clients[active_fd];

                        // On remet son compteur d'inactivité à zéro pour ne pas le déconnecter.
                        current_client.updateLastActivity();

                        // 1. SÉCURITÉ ET CONVERSION (Le piège des données binaires)
                        // On convertit notre tableau brut ('buffer') en un objet C++ moderne ('std::string').
                        // Le fait d'ajouter 'bytes_received' en 2ème paramètre est CRUCIAL : 
                        // Cela force le C++ à copier EXACTEMENT le nombre d'octets reçus par la carte réseau.
                        // Sans ça, si le client envoie un fichier (ex: une image) contenant des octets nuls '\0', 
                        // la copie s'arrêterait en plein milieu et on perdrait la moitié de la requête !
                        std::string received_data(buffer, bytes_received);
                        std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM SOCKET " << active_fd << " ---\n" << received_data << std::endl;

                        // =========================================================================
                        // INTÉGRATION DU PARSEUR HTTP (À FAIRE PAR ROMANE)
                        // =========================================================================
                        // 1. Stocker 'received_data' dans un buffer cumulatif propre au client 
                        //    (ex: current_client.appendRequestString(received_data)).
                        // 2. Appeler le parseur pour analyser ce buffer.
                        // 3. Déterminer si la requête est complète (présence de "\r\n\r\n" ou fin du chunking).
                        
                        // Simulation du retour du parseur HTTP :
                        // - true  : La requête est entière, on peut la traiter.
                        // - false : Il manque des morceaux, on laisse epoll_wait nous réveiller au prochain tour.bool is_request_complete = true; 
                        bool is_request_complete = true;

                        // =========================================================================
                        // LE CERVEAU DU SERVEUR : TRAITEMENT ET BASCULE ASYNCHRONE
                        // =========================================================================
                        if (is_request_complete) {

                            // ÉTAPE A : TRAITEMENT DE LA REQUÊTE
                            // On indique que le serveur est en train de "réfléchir" (vérification 
                            // des droits, recherche du fichier HTML sur le disque, exécution CGI, etc.).
                            current_client.setState(Client::PROCESSING);

                            // ROMANE : C'est ici qu'il faut construire la vraie réponse HTTP complète
                            // (En-têtes + Corps de la page). 
                            // Ex: "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello</h1>"
                            // Cette réponse devra être sauvegardée dans l'objet Client.
                            std::string response = "Good talking to you!\n";
                        
                            // =========================================================
                            // ROMANE : BUFFERISATION DE LA RÉPONSE HTTP
                            // =========================================================
                            // La réponse HTTP (headers + body) doit impérativement être 
                            // persistée dans l'instance du client via cette méthode.
                            // 
                            // Contexte technique (I/O asynchrone) : 
                            // Nous opérons sur des sockets non-bloquants pilotés par epoll. 
                            // Un appel immédiat à send() risquerait de bloquer le thread 
                            // principal (erreur EAGAIN/EWOULDBLOCK) si le buffer d'émission 
                            // du kernel est plein.
                            // 
                            // On sauvegarde donc l'état en mémoire, on bascule le descripteur 
                            // de fichier en EPOLLOUT, et on rend la main à l'Event Loop. 
                            // L'envoi effectif sera déclenché lors du prochain événement epoll.
                            current_client.setResponseBuffer(response);

                            // ÉTAPE B : PRÉPARATION À L'ENVOI
                            // Le traitement est terminé, la réponse est prête en mémoire.
                            current_client.setState(Client::WRITING_RESPONSE);

                            // ÉTAPE C : LA BASCULE (EPOLL_CTL_MOD)
                            // C'est le cœur de l'architecture asynchrone.
                            // On modifie l'événement surveillé par epoll par EPOLLOUT.
                            struct epoll_event mod_ev;

                            // EPOLLOUT indique qu'on veut être réveillé dès que le socket est libre pour ÉCRIRE.
                            mod_ev.events = EPOLLOUT;

                            // ÉTAPE D : IDENTIFICATION DU SOCKET POUR LE RÉVEIL
                            // On attache le descripteur du client (active_fd) à cette nouvelle règle.
                            // Comme ça, quand la carte réseau sera prête, epoll_wait nous réveillera 
                            // en nous redonnant ce numéro exact, et on saura à qui envoyer la réponse.
                            mod_ev.data.fd = active_fd;

                            // ÉTAPE E : L'APPEL SYSTÈME DE MODIFICATION (La bascule officielle)
                            // On demande au noyau Linux de remplacer l'ancienne règle (EPOLLIN) 
                            // par la nouvelle (EPOLLOUT) pour ce socket précis dans l'arbre d'epoll.
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, active_fd, &mod_ev) == -1) {
                                
                                // GESTION D'ERREUR CRITIQUE (Le socket est coincé)
                                // Si la modification échoue (ex: bug système, socket corrompu entre temps),
                                // on affiche l'erreur exacte renvoyée par le noyau.
                                std::cerr << "Error: epoll_ctl(MOD) failed on socket " << active_fd << ": " << std::strerror(errno) << std::endl;
                                
                                // NETTOYAGE
                                // Vu qu'on ne peut pas lui répondre, ce client ne sert plus à rien.
                                // 1. On le débranche d'epoll manuellement.
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);

                                // 2. On ferme la connexion réseau physique pour libérer le port.
                                close(active_fd);

                                // 3. On détruit notre objet Client en mémoire pour éviter les fuites (Leaks).
                                clients.erase(active_fd);

                                // On passe à l'événement suivant, le serveur continue de tourner.
                                continue;
                            }
                            // LOG DE SUCCÈS
                            // Si on arrive ici, la bascule a réussi. Le serveur a fini son travail 
                            // de réflexion et se met en attente (asynchrone) que le réseau se libère.
                            std::cout << "Socket " << active_fd << " successfully switched to EPOLLOUT. Waiting for network to be ready to send..." << std::endl;
                            std::cout << "Socket " << active_fd << " switched to EPOLLOUT. Waiting for network to be ready to send..." << std::endl;
                        }
                    }
                }        
                // =================================================================
                // CAS 3 : LE RÉSEAU EST PRÊT (Émission de la réponse)
                // =================================================================
                // epoll_wait nous avertit avec EPOLLOUT que le système est prêt à 
                // envoyer des données sur le réseau pour ce client précis.
                else if (events[i].events & EPOLLOUT) {
                    // 1. RÉCUPÉRATION DU CONTEXTE CLIENT
                    Client &current_client = clients[active_fd];

                    // =========================================================
                    // ROMANE : LA RÉCUPÉRATION DE LA RÉPONSE (La suite logique)
                    // =========================================================
                    // C'est ici que ton travail de l'étape précédente prend tout son sens !
                    // 
                    // Comme notre serveur ne bloque jamais, on est sortis 
                    // de l'événement de lecture (EPOLLIN) pour attendre que le réseau se libère.
                    // Du coup, toutes les variables locales qu'on avait créées ont été détruites 
                    // à la fin du tour de boucle.
                    // 
                    // C'est pour ça qu'on avait sauvegardé ta réponse finale à l'intérieur 
                    // de l'objet 'Client'. Maintenant qu'on a le feu vert pour écrire (EPOLLOUT), 
                    // on fait simplement appel à getResponseBuffer() pour récupérer ta string 
                    // intacte et l'envoyer avec send().
                    std::string response_to_send;
                    //response_to_send = current_client.getResponseBuffer();
                    response_to_send = "Good talking to you!\n";

                    // =========================================================
                    // 2. L'ENVOI DE LA RÉPONSE
                    // =========================================================
                    // On envoie enfin le texte au client via son socket (active_fd).
                    // epoll nous ayant donné le feu vert, ça partira sans bloquer le serveur.
                    ssize_t bytes_sent = send(active_fd, response_to_send.c_str(), response_to_send.size(), 0);
                    
                    // =========================================================
                    // 3. VÉRIFICATION DE L'ENVOI
                    // =========================================================
                    // Erreur lors de l'envoi (ex: le client a perdu sa connexion d'un coup)
                    if (bytes_sent < 0) {
                        std::cerr << "Error: send() failed on socket " << active_fd << ": " << std::strerror(errno) << std::endl;
                    }
                    // Note : Avec epoll en mode EPOLLOUT, ce cas est très rare.
                    // Cela signifie simplement que rien n'a été envoyé ce coup-ci.
                    else if (bytes_sent == 0) {
                        std::cout << "Notice: 0 bytes sent to socket " << active_fd << " (Network buffer full)" << std::endl;
                        continue;
                    // Succès partiel ou total de l'envoi
                    // ROMANE : vérifier avec une condition dans ce else si la réponse a été totalement envoyée
                    // Si la réponse n'a pas été envoyée totalement, ne pas faire le noettyage final (4. NETTOYAGE FINAL)
                    } else {
                        std::cout << "Successfully sent " << bytes_sent << " bytes back to socket " << active_fd << std::endl;
                        // On met à jour l'activité car le client vient de "communiquer" avec nous
                        current_client.updateLastActivity();
                    }
                    // 4. RÉARMEMENT DU SOCKET (MODE KEEP-ALIVE)
                    // Au lieu de fermer la connexion (close), on prépare le socket à recevoir 
                    // une éventuelle nouvelle requête HTTP sur la même connexion TCP.
                    struct epoll_event listen_ev;
                    // On repasse en mode lecture (EPOLLIN) pour être alerté dès que le client reparlera.
                    listen_ev.events = EPOLLIN;
                    listen_ev.data.fd = active_fd;

                    // On demande à epoll de modifier (MOD) la surveillance de ce socket.
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, active_fd, &listen_ev) == 1000) {
                        // GESTION D'ERREUR : Si la modification échoue (rare), on ne prend aucun 
                        // risque de laisser un socket "fantôme" et on ferme tout proprement.
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                        close(active_fd);
                        clients.erase(active_fd);
                    } else {
                        // SUCCÈS DU KEEP-ALIVE : Le socket TCP reste ouvert.
                        // On réinitialise l'état du client pour repartir sur un nouveau cycle de lecture.
                        
                        // ROMANE : C'est ici qu'il faudra vider les buffers (requête et réponse) 
                        // pour ne pas mélanger l'ancienne requête avec la nouvelle.
                        // current_client.clearBuffers();
                        current_client.setState(Client::READING_REQUEST);
                        std::cout << "Socket " << active_fd << " kept alive. Waiting for next request..." << std::endl;
                    }
                }
            }
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        if (sockfd != -1) {
            close(sockfd);
        }
        return (1);
    }
    std::cout << "Shutting down the server (sockfd)." << std::endl;
    if (sockfd != -1) {
        close(sockfd);
    }
    return (0);
}
*/