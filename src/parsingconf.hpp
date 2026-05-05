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
void	validateIP(std::string str);
void	validateOneArg(std::string str, ServerConfig& srv);
void	validateListen(std::vector<std::string> args, State state, ServerConfig& srv);
void	validateRoot(std::vector<std::string> args);
void	validateClientMaxBodySize(std::vector<std::string> args);
bool	isErrorCode(std::string code);
void	validateErrorPage(std::vector<std::string> args);
void	isValidUrl(const std::string& url);
void	validateReturn(std::vector<std::string> args);
bool	validateIndex(std::vector<std::string> args);
bool	validateAutoIndex(std::vector<std::string> args, State state, ServerConfig& srv);
bool	validateAllowedMethods(std::vector<std::string> args, ServerConfig& srv, State state);
bool	validateAllowedUpload(std::vector<std::string> args, ServerConfig& srv, State state);
bool	validateUploadPath(std::vector<std::string> args,ServerConfig& srv, State state);
bool	validateSpecificDirective(std::string name, std::vector<std::string> args, State state, ServerConfig& srv);

std::vector<std::string> combineRootUri(std::string root, std::string uri);

