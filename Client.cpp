#include "Client.hpp"
#include <cstring> 
Client::Client() : socket_fd_(-1), state_(READING_REQUEST), last_activity_(time(NULL)), ip_address_("") {
    memset(&addr_, 0, sizeof(addr_));
}

Client::Client(int socket_fd, struct sockaddr_storage addr) : socket_fd_(socket_fd), addr_(addr), state_(READING_REQUEST), last_activity_(time(NULL)) {
    _initIpAddress(addr);
}

void Client::_initIpAddress(struct sockaddr_storage addr) {
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

Client::Client(const Client &src) : socket_fd_(src.socket_fd_), addr_(src.addr_), state_(src.state_), last_activity_(src.last_activity_), ip_address_(src.ip_address_) {

}

Client &Client::operator=(const Client &rhs) {
    if (this != &rhs) {
        socket_fd_ = rhs.socket_fd_;
        addr_ = rhs.addr_;
        state_ = rhs.state_;
        last_activity_ = rhs.last_activity_;
        ip_address_ = rhs.ip_address_;
    }
    return (*this);
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

std::string Client::getIp() const {
    return (this->ip_address_);
}

Client::~Client() {

}