#pragma once

#include <string>
#include <vector>
#include "ServerConfig.hpp"

enum State {
    OUTSIDE,
    IN_SERVER,
    IN_LOCATION
};

std::string readFile(const char *path);

std::vector<std::string> tokenizeConfig(std::string str);

bool validateStructure(std::vector<std::string> &tokens, std::vector<ServerConfig> &all_servers);

bool isSimpleDirective(std::string name);
bool directiveIsAllowed(std::string name, State state);
bool validateOneDirective(std::vector<std::string> tokens, size_t& i, State state, ServerConfig& srv);