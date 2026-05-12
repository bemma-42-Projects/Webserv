#include "Client.hpp"

#include <cstring>

// constructeur par défaut
Client::Client() : socket_fd_(-1), state_(READING_REQUEST), last_activity_(time(NULL)), ip_address_(""), request_buffer_(), response_buffer_(), request_(), answer_(), config_() {
    memset(&addr_, 0, sizeof(addr_));
}

Client::Client(int socket_fd, struct sockaddr_storage addr, const ServerConfig *config) : socket_fd_(socket_fd), addr_(addr), state_(READING_REQUEST), last_activity_(time(NULL)),  ip_address_(""), request_buffer_(), response_buffer_(), request_(), answer_(), config_(config) {
    initIpAddress_(addr);
}

void Client::initIpAddress_(struct sockaddr_storage addr) {
    char ip_buffer[INET6_ADDRSTRLEN];
    void *raw_ip_ptr;
    std::string ip_version;
    if (addr.ss_family == AF_INET) {
        raw_ip_ptr = &(reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr);
        ip_version = "IPv4";
    } else {
        raw_ip_ptr = &(reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr);
        ip_version = "IPv6";
    }
    inet_ntop(addr.ss_family, raw_ip_ptr, ip_buffer, sizeof(ip_buffer));
    this->ip_address_ = std::string(ip_buffer) + " (" + ip_version + ")";
}

int Client::getSocketFd() const {
    return (this->socket_fd_);
}

Client::State Client::getState() const {
    return (this->state_);
}

void Client::setState(State state) {
    this->state_ = state;
}

time_t Client::getLastActivity() const {
    return (this->last_activity_);
}

void Client::updateLastActivity() {
    this->last_activity_ = time(NULL);
}

const ServerConfig  *Client::getConfig() const {
    return (this->config_);
}

std::string Client::getIp() const {
    return (this->ip_address_);
}

// pont entre Server et Request
// le serveur lit le réseau avec epoll
// s'il y a des données, il fait un read et obtient un buffer
// le serveur doit utiliser ce getter pour accéder à la fonction de parsing
// de Request !
// et aussi vérifier s'il faut continuer de lire ou préparer la réponse
Request &Client::getRequest()
{
    return (this->request_);
}

// pont entre server et RequestAnswer
// une fois que le parsing de la requete est un succes
// le serveur doit ordonner la creation de la reponse
// c'est grace a cette fonction que le serveur
// accede a la reponse
RequestAnswer   &Client::getAnswer()
{
    return (this->answer_);
}

// lorsque le navigateur envoie une requete, elle peut etre decoupee en plusieurs paquets
// il faut donc remplir le buffer a chaque fois que epoll a detecte
// que le client a envoye quelque-chose
// (quand on recoit un event EPOLLIN)
void    Client::appendRequestData(const std::string &data)
{
    this->request_buffer_ += data;
}

// pour acceder au buffer, et voir s'il reste des donnees ou non
const std::string   &Client::getRequestData() const {
    return (this->request_buffer_);
}

void    Client::clearBuffers()
{
    this->request_buffer_.clear();
    this->response_buffer_.clear();

    // Ajoute un petit debug ici aussi pour vérifier que c'est appelé
    this->request_.clear();
    this->answer_.clear();

    this->last_activity_ = time(NULL);
}

Client::~Client()
{

}
