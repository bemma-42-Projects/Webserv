#include "Config.hpp"

Config::Config()
{}
Config::~Config()
{}

bool Config::autoindex_ = true;

void Config::setAutoindex(bool value)
{
	(void)value;
	autoindex_ = true;
}

bool	Config::getAutoindex()
{
	return autoindex_;
}