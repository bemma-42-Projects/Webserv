#include "Client.hpp"
#include <cstring> // Pour memset

// Constructeur par défaut
// On initialise le socket à -1 pour indiquer qu'il n'est pas encore connecté
Client::Client() : _socket_fd(-1), _state(READING_REQUEST), _last_activity(time(NULL)), _ip_address("") {
    // Initialisation de l'adresse à zéro
    memset(&_addr, 0, sizeof(_addr));
}

// Constructeur paramétrique
Client::Client(int socket_fd, struct sockaddr_storage addr) : _socket_fd(socket_fd), _addr(addr), _state(READING_REQUEST), _last_activity(time(NULL)) {
    _initIpAddress(addr);
}

// =========================================================================================
// EXTRACTION ET TRADUCTION DE L'IP DU CLIENT (ACCESS LOGS)
// =========================================================================================
// L'adresse IP du client a été capturée par accept() sous un format binaire illisible 
// dans la structure générique 'client_addr'. Ce bloc sert à traduire ce format 
// binaire en une chaîne de caractères (ex: "192.168.1.15").
void Client::_initIpAddress(struct sockaddr_storage addr) {
    // INET6_ADDRSTRLEN est une constante du système qui définit la taille maximale 
    // nécessaire pour stocker la plus longue adresse IPv6 possible sous forme de texte.
    char ip_buffer[INET6_ADDRSTRLEN];

    // Un pointeur générique qui pointera vers la donnée binaire de l'IP, qu'elle soit v4 ou v6.
    void *raw_ip_ptr;

    // Stockera "IPv4" ou "IPv6" pour l'affichage
    std::string ip_version;

    // Identification de la version de l'IP
    // Si c'est une adresse IPv4
    if (addr.ss_family == AF_INET) {
        // Cast de l'adresse générique en structure IPv4 et extraction de l'adresse IPv4 brute (sin_addr)
        raw_ip_ptr = &(reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr);
        // Mise à jour de la variable d'affichage pour indiquer IPv4
        ip_version = "IPv4";
    // Sinon, c'est une adresse IPv6 (AF_INET6)
    } else {
        // Cast de l'adresse générique en structure IPv6 et extraction de l'adresse IPv6 brute (sin6_addr)
        raw_ip_ptr = &(reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr);
        // Mise à jour de la variable d'affichage pour indiquer IPv6
        ip_version = "IPv6";
    }

    // Conversion de l'adresse IP brute en une chaîne de caractères lisible
    // inet_ntop (Network TO Presentation) : Binaire -> Texte
    //
    // Paramètres :
    // 1. Famille (addr.ss_family) : IPv4 ou IPv6.
    // 2. Source (raw_ip_ptr) : Le pointeur vers l'adresse binaire brute.
    // 3. Destination (ip_buffer) : Le tableau où écrire le texte.
    // 4. Taille (sizeof(ip_buffer)) : Pour éviter tout débordement mémoire.
    inet_ntop(addr.ss_family, raw_ip_ptr, ip_buffer, sizeof(ip_buffer));

    // Stockage de l'IP du client dans l'objet Client
    this->_ip_address = std::string(ip_buffer) + " (" + ip_version + ")";
}

// Constructeur par copie
Client::Client(const Client &copy) : _socket_fd(copy._socket_fd), _addr(copy._addr), _state(copy._state), _last_activity(copy._last_activity), _ip_address(copy._ip_address) {
}

// Surcharge de l'opérateur d'affectation
Client &Client::operator=(const Client &src) {
    if (this != &src) {
        _socket_fd = src._socket_fd;
        _addr = src._addr;
        _state = src._state;
        _last_activity = src._last_activity;
        _ip_address = src._ip_address;
    }
    return (*this);
}

int Client::getSocketFd() const {
    return (this->_socket_fd);
}

Client::State Client::getState() const {
    return (this->_state);
}

void Client::setState(State state) {
    this->_state = state;
}

time_t Client::getLastActivity() const {
    return (this->_last_activity);
}

void Client::updateLastActivity() {
    this->_last_activity = time(NULL);
}



// Destructeur
Client::~Client() {

}