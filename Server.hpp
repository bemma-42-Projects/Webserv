#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <sys/epoll.h>

# include "Client.hpp"

# define MAX_EVENTS 64

extern bool g_running;

class   Server {
    public:
        /**
         * @brief Constructeur par défaut du serveur.
         * * @details Initialise les variables membres de base. Ne lance aucune opération réseau bloquante.
         * * @note S'utilise simplement lors de l'instanciation : `Server myServer;`
         */
        Server();

        /**
         * @brief Destructeur du serveur.
         * * @details Parcours et ferme proprement tous les sockets encore ouverts (le socket serveur principal 
         * et ceux des clients connectés), puis ferme l'instance epoll pour éviter les fuites de FD.
         * * @note Appelé automatiquement à la destruction de l'objet.
         */
        ~Server();

        /**
         * @brief Initialise toute l'infrastructure réseau du serveur.
         * * @details Orchestre la création du socket principal, sa liaison (bind) à l'adresse/port, 
         * la mise en écoute (listen) et la création de l'instance epoll pour la gestion asynchrone.
         * * @note À appeler explicitement une seule fois après l'instanciation et avant de lancer `run()`.
         */
        void    init();

        /**
         * @brief Lance la boucle d'événements principale du serveur.
         * * @details Utilise `epoll_wait` en boucle (tant que `g_running` est vrai) pour intercepter 
         * les événements réseau (nouvelle connexion, données prêtes à être lues, socket prêt pour l'écriture) 
         * et délègue le travail aux sous-fonctions appropriées. Gère également les timeouts.
         * * @note Constitue le cœur du programme. Bloque l'exécution du thread principal jusqu'à l'arrêt du serveur.
         */
        void    run();

    private:
        /* Forme Canonique Orthodoxe (Privée pour empêcher la copie) */
        Server(const Server &src);
        Server &operator=(const Server &rhs);

        /**
         * @brief Rend un file descriptor non-bloquant.
         * * @details Utilise la fonction système `fcntl` avec les flags `F_GETFL` et `F_SETFL` pour ajouter 
         * l'attribut `O_NONBLOCK` au socket.
         * * @note S'utilise sur le socket serveur et chaque nouveau socket client pour s'assurer que les appels 
         * `recv` et `send` ne bloquent jamais la boucle `epoll`.
         */
        void            _setNonBlocking(int fd);

        /**
         * @brief Initialise les "hints" pour `getaddrinfo`.
         * * @details Remplit la structure `addrinfo` avec des zéros et définit la famille d'adresses 
         * (ex: AF_UNSPEC pour IPv4/IPv6) et le type de socket (SOCK_STREAM pour TCP).
         * * @note Fonction utilitaire appelée par `_createAndBindSocket` avant de résoudre le port.
         */
        void            _initAddrinfoParams(struct addrinfo &addrinfo_params);
        
        /**
         * @brief Récupère les informations d'adresse système.
         * * @details Fait appel à `getaddrinfo()` en utilisant les paramètres initialisés pour obtenir 
         * une liste chaînée d'interfaces réseau sur lesquelles le serveur peut se binder.
         * * @note Retourne un pointeur alloué dynamiquement qu'il faudra libérer avec `freeaddrinfo()`.
         */
        struct addrinfo *_getAddrInfo(const std::string &port_str);
        
        /**
         * @brief Affiche l'adresse IP de l'interface réseau (Log/Debug).
         * * @details Convertit l'adresse binaire de la structure `addrinfo` en chaîne de caractères lisible 
         * (via `inet_ntop`) et l'imprime.
         * * @note S'utilise pour informer l'utilisateur de l'IP sur laquelle le serveur écoute.
         */
        void            _printInterface(struct addrinfo *p, char *ip_buffer);
        
        /**
         * @brief Tente de créer un socket et de configurer ses options.
         * * @details Appelle `socket()`, puis `setsockopt()` avec `SO_REUSEADDR` pour permettre au serveur 
         * de redémarrer rapidement sans attendre que le port soit libéré par l'OS. 
         * * @note Utilisé au sein de `_bindSocketLoop` pour tester chaque interface disponible.
         */
        bool            _setupSocket(struct addrinfo *p, const std::string &port_str);
        
        /**
         * @brief Boucle sur les interfaces réseau pour binder le serveur.
         * * @details Parcourt la liste chaînée retournée par `getaddrinfo`. Pour chaque élément, tente de 
         * créer le socket avec `_setupSocket` et de le lier avec `bind()`. S'arrête dès qu'un bind réussit.
         * * @note Garantit que le serveur trouve une interface valide pour démarrer.
         */
        void            _bindSocketLoop(struct addrinfo *res, const std::string &port_str);
        
        /**
         * @brief Orchestre la création et le bind du socket principal.
         * * @details Appelle dans l'ordre : `_initAddrinfoParams`, `_getAddrInfo`, puis `_bindSocketLoop`. 
         * Nettoie également la mémoire allouée par `getaddrinfo`.
         * * @note C'est le point d'entrée de la configuration réseau, appelé directement par `init()`.
         */
        void            _createAndBindSocket(const std::string &port_str);
        
        /**
         * @brief Met le socket serveur en mode écoute.
         * * @details Appelle la fonction système `listen()` avec une valeur de backlog appropriée pour 
         * définir la taille de la file d'attente des connexions entrantes.
         * * @note À appeler juste après que le socket soit bindé avec succès.
         */
        void            _startListening();

        /**
         * @brief Initialise l'instance de multiplexage epoll.
         * * @details Appelle `epoll_create1()`, puis inscrit le socket serveur (`_server_socket`) dans 
         * l'instance epoll en l'écoutant pour les événements de lecture (`EPOLLIN`).
         * * @note Dernière étape de `init()` avant de pouvoir lancer la boucle d'événements.
         */
        void            _initEpoll();
        
        /**
         * @brief Déconnecte les clients inactifs.
         * * @details Parcourt la `std::map` `_clients` et compare le timestamp de la dernière activité 
         * de chaque client avec le temps actuel. Si la différence dépasse un seuil, le client est supprimé.
         * * @note Appelé périodiquement dans la boucle `run()` pour éviter que des clients fantômes ne 
         * saturent les ressources du serveur.
         */
        void            _handleTimeouts();

        /**
         * @brief Gère la déconnexion propre d'un client.
         * * @details Ferme le socket du client, le retire du système epoll (`epoll_ctl` avec EPOLL_CTL_DEL), 
         * et efface l'objet `Client` de la map `_clients`.
         * * @note À appeler dès qu'une erreur de lecture/écriture survient (recv/send retournant <= 0) ou en cas de timeout.
         */
        void            _handleClientDisconnect(int client_fd);
        
        /**
         * @brief Ajoute un nouveau socket client à la surveillance epoll.
         * * @details Configure une structure `epoll_event` pour écouter les événements de lecture (`EPOLLIN`) 
         * sur le socket spécifié, puis l'ajoute avec `epoll_ctl` (EPOLL_CTL_ADD).
         * * @note Utilisé immédiatement après avoir accepté une nouvelle connexion.
         */
        bool            _addClientToEpoll(int client_fd);
        
        /**
         * @brief Affiche un message de journalisation pour une nouvelle connexion.
         * * @details Fonction utilitaire de debug pour afficher dans la console qu'un nouveau client (avec son FD) 
         * s'est connecté.
         */
        void            _logNewConnection(int client_fd);
        
        /**
         * @brief Accepte et configure une nouvelle connexion cliente entrante.
         * * @details Appelle `accept()` sur le socket serveur, rend le nouveau socket non-bloquant, 
         * l'ajoute à epoll et instancie un nouvel objet `Client` dans `_clients`.
         * * @note Appelé par `run()` quand epoll signale un événement de lecture sur le `_server_socket`.
         */
        void            _handleNewConnection();

        /**
         * @brief Bascule la surveillance epoll d'un client en mode écriture.
         * * @details Utilise `epoll_ctl` (EPOLL_CTL_MOD) pour modifier les flags du client de `EPOLLIN` 
         * vers `EPOLLOUT`.
         * * @note À appeler une fois que la requête HTTP est complètement reçue et que la réponse 
         * est prête à être envoyée.
         */
        void            _setSocketToWriteState(int client_fd);
        
        /**
         * @brief Vérifie si la requête HTTP du client est complètement reçue.
         * * @details Analyse le buffer de lecture du client. Cherche la fin des headers (`\r\n\r\n`), 
         * vérifie le `Content-Length` ou le format `chunked` pour savoir si le body est complet.
         * * @note Appelé à chaque fois que de nouvelles données sont lues sur le socket client.
         */
        bool	        _isRequestComplete(Client &client);
        
        /**
         * @brief Génère la réponse HTTP appropriée.
         * * @details Analyse la requête parsée (méthode, URI, headers), interagit avec le système de fichiers 
         * ou les scripts CGI si nécessaire, et construit le texte brut (Headers + Body) de la réponse HTTP.
         * * @note Appelé une fois que `_isRequestComplete` retourne `true`.
         */
        std::string     _buildHttpResponse(Client &client);
        
        /**
         * @brief Stocke la réponse générée dans le buffer d'écriture du client.
         * * @details Prend la chaîne de réponse complète et l'affecte au buffer interne du `Client` 
         * pour qu'elle puisse être envoyée progressivement.
         * * @note S'utilise en conjonction avec `_setSocketToWriteState`.
         */
        void            _bufferizeResponse(Client& client, const std::string& response);
        
        /**
         * @brief Traite les données brutes reçues d'un client.
         * * @details Ajoute les nouvelles données au buffer de lecture du client. Si la requête est complète, 
         * déclenche la construction de la réponse et le basculement en mode écriture.
         * * @note Séparation logique pour parser/traiter les données en dehors de la logique pure de `recv`.
         */
        void            _processClientRequest(int client_fd, const std::string& received_data);
        
        /**
         * @brief Gère l'événement de lecture sur un socket client.
         * * @details Appelle `recv()` de manière non-bloquante, vérifie les déconnexions (0) ou les erreurs (-1), 
         * et passe les données lues à `_processClientRequest()`.
         * * @note Appelé par `run()` lorsque epoll signale `EPOLLIN` sur un FD de client.
         */
        void            _handleClientRead(int client_fd);
        
        /**
         * @brief Récupère le prochain bloc de données à envoyer au client.
         * * @details Extrait une sous-chaîne du buffer d'écriture du client, en respectant la limite 
         * de taille maximale gérable par `send()` en une seule fois.
         * * @note Utilisé au sein de `_handleClientWrite`.
         */
        std::string     _getResponseToSend(Client& client);
        
        /**
         * @brief Vérifie si la réponse entière a été transmise.
         * * @details Met à jour l'index ou efface les octets déjà envoyés du buffer d'écriture en fonction 
         * de `bytes_sent`. Retourne `true` si le buffer est désormais vide.
         * * @note Permet de savoir s'il faut repasser le socket en mode lecture (`EPOLLIN`).
         */
        bool            _isResponseFullySent(Client& client, ssize_t bytes_sent);
        
        /**
         * @brief Réinitialise l'état du client après une transaction.
         * * @details Vide les buffers de lecture et d'écriture, et réinitialise les variables de parsing 
         * HTTP de l'objet `Client` pour qu'il soit prêt à recevoir une nouvelle requête (Keep-Alive).
         * * @note Appelé une fois que la réponse est totalement envoyée.
         */
        void            _clearClientBuffers(Client &client);
        
        /**
         * @brief Bascule la surveillance epoll d'un client en mode lecture.
         * * @details Utilise `epoll_ctl` (EPOLL_CTL_MOD) pour modifier les flags du client de `EPOLLOUT` 
         * vers `EPOLLIN`.
         * * @note À appeler une fois la réponse envoyée pour attendre la prochaine requête (Keep-Alive).
         */
        void            _setSocketToReadState(int client_fd);
        
        /**
         * @brief Gère l'événement d'écriture sur un socket client.
         * * @details Appelle `send()` avec la partie de la réponse prête à partir. Gère les envois partiels. 
         * Si la réponse est totalement envoyée, nettoie les buffers et repasse le socket en lecture.
         * * @note Appelé par `run()` lorsque epoll signale `EPOLLOUT` sur un FD de client.
         */
        void            _handleClientWrite(int client_fd);

        int server_socket_;
        int epoll_fd_;
        std::map<int, Client> _clients;
};

#endif