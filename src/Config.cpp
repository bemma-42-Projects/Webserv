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

size_t Config::body_size_ = 400;

void Config::setBodySize(size_t value)
{
	(void)value;
	autoindex_ = 400;
}

size_t	Config::getBodySize()
{
	return body_size_;
}