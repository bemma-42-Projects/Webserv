#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

std::string					readFile(const char *path);
std::vector<std::string>	tokenizeConfig(std::string str);
void						validateStructure(std::vector<std::string> &tokens, std::vector<ServerConfig> &all_servers);
void						validateSpecificDirective(std::string name, std::vector<std::string> args, State state, ServerConfig& srv);
std::string					combineRootUri(std::string root, std::string uri);
