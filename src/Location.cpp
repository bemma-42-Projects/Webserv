#include "Location.hpp"

//Location::Location()
//{
//}

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
	return (this->index_);
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
	return (this->allowed_methods_);
}

void	Location::addCgiHandler(std::string ext, std::string interpreter)
{
	this->cgi_handlers_[ext] = interpreter;
}

std::map<std::string, std::string>	Location::getCgiHandlers() const
{
	return (this->cgi_handlers_);
}
