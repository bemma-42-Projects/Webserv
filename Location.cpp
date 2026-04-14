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
	path_ = path;
	root_ = root;
	upload_path_ = upload_path;
	index_ = index;
	autoindex_ = autoindex;

	// C'est ici que le push_back est autorisé
	allowed_methods_ = methods;
}

std::vector<std::string>	Location::getIndex()
{
	return index_;
}

bool	Location::getAutoindex()
{
	return autoindex_;
}

std::string	Location::getRoot()
{
	return root_;
}

std::string	Location::getPath()
{
	return path_;
}

std::vector<std::string>	Location::getAllowedMethods()
{
	return allowed_methods_;
}

void	Location::addCgiHandler(std::string ext, std::string interpreter)
{
	cgi_handlers_[ext] = interpreter;
}

std::map<std::string, std::string>	Location::getCgiHandlers() const
{
	return cgi_handlers_;
}
