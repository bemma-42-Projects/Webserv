#include "WebservException.hpp"

// Constructeur
WebservException::WebservException(const std::string &message) : _message(message) {
}

// Destructeur
WebservException::~WebservException() throw() {
}

// Surcharge de what()
const char* WebservException::what() const throw() {
    return _message.c_str();
}
