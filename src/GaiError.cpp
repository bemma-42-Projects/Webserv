#include "GaiError.hpp"
#include <netdb.h>

GaiError::GaiError(const std::string &context, int errcode) {
    _message = context + ": " + std::string(gai_strerror(errcode));
}

GaiError::~GaiError() throw() {}

const char *GaiError::what() const throw() {
    return _message.c_str();
}