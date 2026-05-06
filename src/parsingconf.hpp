#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>
#include <cctype>
#include <stack>
#include <cstdlib>
#include <set>

#include <sys/stat.h>

#include "ServerConfig.hpp"

enum State {
	OUTSIDE,
	IN_SERVER,
	IN_LOCATION
};


int		validatePort(std::string port_str);
bool	validateIP(std::string str);
bool	validateOneArg(std::string str, ServerConfig& srv);
bool	validateListen(std::vector<std::string> args, State state, ServerConfig& srv);
bool	validateRoot(std::vector<std::string> args);
bool	validateClientMaxBodySize(std::vector<std::string> args);
bool	isErrorCode(std::string code);
bool	validateErrorPage(std::vector<std::string> args);
bool	isValidUrl(const std::string& url);
bool	validateReturn(std::vector<std::string> args);
bool	validateIndex(std::vector<std::string> args);
bool	validateAutoIndex(std::vector<std::string> args, State state, ServerConfig& srv);
bool	validateAllowedMethods(std::vector<std::string> args, ServerConfig& srv, State state);
bool	validateAllowedUpload(std::vector<std::string> args, ServerConfig& srv, State state);
bool	validateUploadPath(std::vector<std::string> args,ServerConfig& srv, State state);
bool	validateSpecificDirective(std::string name, std::vector<std::string> args, State state, ServerConfig& srv);

std::vector<std::string> combineRootUri(std::string root, std::string uri);

