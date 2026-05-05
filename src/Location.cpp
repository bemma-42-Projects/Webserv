#include "Location.hpp"
//#include "Config.hpp"

Location::Location() : path_(""), root_(""), autoindex_(false) {
}

Location::~Location()
{}

Location::Location(std::string path, std::string root, 
		std::string upload_path, std::vector<std::string> index, 
		bool autoindex, std::vector<std::string> methods)
{
	this->path_ = path;
	this->root_ = root;
	this->upload_path_ = upload_path;
	this->index_ = index;
	this->autoindex_ = autoindex;

	// C'est ici que le push_back est autorisé
	this->allowed_methods_ = methods;
}

std::vector<std::string>	Location::getIndex() const
{
	//if (!index_.empty())
	return index_;
	//return Config::getIndex();
}

bool	Location::getAutoindex() const
{
	return (this->autoindex_);
}

std::string	Location::getRoot() const
{
	return (this->root_);
}

std::string	Location::getPath() const
{
	return (this->path_);
}

std::vector<std::string>	Location::getAllowedMethods() const
{
	return allowed_methods_;
}

std::map<int, std::string>	Location::getError()
{
    error_[404] = "./er.txt";
	return error_;
}