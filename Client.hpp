#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <ctime>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

class   Client {
    public:
        /**
         * @brief Énumération des différents états possibles du client.
         * * @details Permet au serveur (via epoll) de savoir exactement quoi faire avec ce client 
         * à un instant T (lire la requête, générer la réponse, envoyer la réponse, ou le déconnecter).
         */
        enum State {
            READING_REQUEST,
            PROCESSING,
            WRITING_RESPONSE,
            DISCONNECTED
        };

        /**
         * @brief Constructeur par défaut.
         * * @details Initialise un client "vide" avec un FD invalide (-1) et un état par défaut.
         * * @note Nécessaire pour pouvoir utiliser l'objet dans des conteneurs comme `std::map`.
         */
        Client();

        /**
         * @brief Constructeur paramétré (Recommandé).
         * * @details Initialise le client avec son socket fraîchement accepté par le serveur, enregistre 
         * les informations de son adresse, configure son état initial (READING_REQUEST) et met à jour 
         * son timestamp d'activité.
         * * @note À utiliser juste après l'appel à `accept()` dans le serveur.
         */
        Client(int socket_fd, struct sockaddr_storage addr);
        
        /**
         * @brief Constructeur par copie.
         * * @details Copie les attributs d'un client source vers une nouvelle instance.
         */
        Client(const Client &src);

        /**
         * @brief Opérateur d'assignation.
         * * @details Assigne les valeurs d'un client existant à un autre (Forme Canonique Orthodoxe).
         */
        Client  &operator=(const Client &rhs);
        
        /**
         * @brief Destructeur.
         * * @details Détruit l'instance du client. Ne ferme généralement pas le FD ici pour laisser 
         * le contrôle explicite au Server (pour éviter les double-closes ou la fermeture accidentelle lors des copies).
         */
        ~Client();

        /**
         * @brief Récupère le descripteur de fichier du socket client.
         * * @details Retourne l'entier représentant le socket de communication.
         * * @note Utilisé par le serveur pour les appels à `recv()`, `send()` ou `epoll_ctl()`.
         */
        int             getSocketFd() const;

        /**
         * @brief Récupère l'état actuel du client.
         * * @details Retourne la valeur de l'enum `State` (lecture, écriture, etc.).
         * * @note Utile pour le serveur pour vérifier si le client a fini de lire ou d'écrire.
         */
        State           getState() const;

        /**
         * @brief Modifie l'état actuel du client.
         * * @details Met à jour la variable d'état interne.
         * * @note À appeler quand le client passe d'une étape à une autre (ex: de READING_REQUEST à WRITING_RESPONSE).
         */
        void            setState(State state);

        /**
         * @brief Récupère le timestamp de la dernière action du client.
         * * @details Retourne un `time_t` représentant le moment où le client a interagi pour la dernière fois.
         * * @note Utilisé par le serveur pour détecter les timeouts et déconnecter les clients inactifs.
         */
        time_t          getLastActivity() const;

        /**
         * @brief Actualise le chronomètre d'activité du client.
         * * @details Remplace l'ancienne valeur de `_last_activity` par l'heure actuelle du système (`time(NULL)`).
         * * @note Doit être appelé à chaque fois que le serveur reçoit ou envoie avec succès des données à ce client.
         */
        void            updateLastActivity();

        /**
         * @brief Récupère l'adresse IP du client sous forme de texte.
         * * @details Retourne la chaîne de caractères (ex: "192.168.1.10") générée lors de la connexion.
         * * @note Utile pour les logs du serveur ou pour les variables d'environnement CGI (REMOTE_ADDR).
         */
        std::string     getIp() const;

    private:
        int socket_fd_;
        struct sockaddr_storage addr_;
        State state_;
        time_t last_activity_;
        std::string ip_address_;
        /**
         * @brief Buffer d'accumulation de la requête entrante.
         * @details Stocke les paquets de données lus via `recv()` à chaque événement EPOLLIN.
         * Les données s'y accumulent de manière asynchrone jusqu'à ce que la requête HTTP 
         * soit validée comme complète (détection de "\r\n\r\n" et réception intégrale du body).
         */
        std::string request_buffer_;

        /**
         * @brief Buffer de stockage de la réponse sortante.
         * @details Contient la réponse HTTP complète (en-têtes et body) générée par le serveur.
         * Les données sont expédiées via `send()` lors des événements EPOLLOUT. En cas d'envoi
         * partiel sur le réseau, les octets transmis sont progressivement retirés du début de 
         * ce buffer jusqu'à ce qu'il soit totalement vide.
         */
        std::string response_buffer_;
    
        /**
         * @brief Convertit la structure d'adresse en IP lisible.
         * * @details Analyse la structure `sockaddr_storage` pour déterminer s'il s'agit d'une adresse IPv4 ou IPv6, 
         * extrait l'IP binaire, et utilise `inet_ntop` pour la convertir en `std::string` stockée dans `_ip_address`.
         * * @note Méthode utilitaire appelée uniquement par le constructeur paramétré lors de l'instanciation.
         */
        void        _initIpAddress(struct sockaddr_storage addr);

        /**
         * @brief Ajoute les données reçues au buffer de la requête.
         * * @details Concatène la nouvelle chaîne `data` à la suite de `request_buffer_` 
         * en utilisant l'opérateur `+=`. La classe `std::string` gère automatiquement 
         * l'allocation dynamique de la mémoire et le redimensionnement.
         * * @note Appelée par le serveur à chaque fois que `recv()` lit avec succès 
         * un nouveau paquet de données sur le socket (lors d'un événement EPOLLIN).
         */
        void        appendRequestData(const std::string &data);

        /**
         * @brief Récupère l'intégralité des données brutes de la requête.
         * * @details Permet d'accéder au contenu du `request_buffer_` accumulé jusqu'à présent. 
         * Cette référence constante est transmise à la classe `Request` pour vérification et parsing,
         * évitant ainsi une copie coûteuse en mémoire.
         * * @return Une référence constante vers la chaîne `request_buffer_`.
         */
        const std::string &getRequestData() const;

        /**
         * @brief Stocke la réponse HTTP complète prête à être envoyée.
         * * @details Remplace le contenu actuel du `response_buffer_` par la nouvelle chaîne `data`.
         * Contrairement à la réception de requêtes, la réponse est générée d'un seul bloc par le serveur,
         * d'où l'utilisation de l'assignation (`=`) plutôt que de la concaténation (`+=`).
         * * @param data La référence constante vers la chaîne contenant les en-têtes et le corps de la réponse.
         * * @note Appelée une fois que le traitement de la requête est terminé, juste avant 
         * de basculer le socket du client en mode écriture (EPOLLOUT).
         */
        void    setResponseData(const std::string &data);

        /**
         * @brief Supprime les données déjà envoyées du buffer de réponse.
         * * @details Gère les envois partiels inhérents aux sockets non-bloquants. 
         * Si `send()` ne parvient pas à transmettre l'intégralité du `response_buffer_` 
         * en une seule fois, cette méthode supprime du début de la chaîne le nombre 
         * d'octets spécifié. Le reste de la réponse sera envoyé lors du prochain 
         * événement EPOLLOUT.
         * * @param bytes_sent Le nombre d'octets que la fonction système `send()` a 
         * réussi à transmettre au client sur le réseau.
         */
        void        eraseSentResponseData(ssize_t bytes_sent);

        /**
         * @brief Réinitialise le client pour accueillir une nouvelle requête (Keep-Alive).
         * * @details Vide complètement les buffers de requête et de réponse en utilisant 
         * la méthode `.clear()` (qui préserve la capacité mémoire pour optimiser 
         * les allocations futures). Bascule ensuite l'état du client sur `READING_REQUEST`.
         * * @note Cette méthode est appelée exclusivement par le serveur lorsque l'intégralité 
         * de la réponse HTTP a été transmise avec succès (quand `response_buffer_` est vide), 
         * évitant ainsi de fermer la connexion TCP.
         */
        void    clearBuffers();
    };

#endif