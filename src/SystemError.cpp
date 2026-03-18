#include "SystemError.hpp"

SystemError::SystemError(const std::string &context) {
	if (errno != 0) {
		_message = context + ": " + std::string(strerror(errno));
	}
	else {
		_message = context;
	}
}

SystemError::~SystemError() throw() {}

const char	*SystemError::what() const throw() {
	return (_message.c_str());
}
