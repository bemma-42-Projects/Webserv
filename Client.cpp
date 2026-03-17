#include "Client.hpp"
#include <cstring> 
Client::Client() : _socket_fd(-1), _state(READING_REQUEST), _last_activity(time(NULL)), _ip_address("") {
    memset(&_addr, 0, sizeof(_addr));
}

Client::Client(int socket_fd, struct sockaddr_storage addr) : _socket_fd(socket_fd), _addr(addr), _state(READING_REQUEST), _last_activity(time(NULL)) {
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
    this->_ip_address = std::string(ip_buffer) + " (" + ip_version + ")";
}

Client::Client(const Client &src) : _socket_fd(src._socket_fd), _addr(src._addr), _state(src._state), _last_activity(src._last_activity), _ip_address(src._ip_address) {

}

Client &Client::operator=(const Client &rhs) {
    if (this != &rhs) {
        _socket_fd = rhs._socket_fd;
        _addr = rhs._addr;
        _state = rhs._state;
        _last_activity = rhs._last_activity;
        _ip_address = rhs._ip_address;
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

std::string Client::getIp() const {
    return (this->_ip_address);
}

Client::~Client() {

}