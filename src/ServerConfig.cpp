#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {
	autoindex_ = false;
	client_max_body_size_ = 1048576;
	// Listen newlisten;
	// listen_.push_back(newlisten);
	
}

const std::vector<Listen>&  ServerConfig::getListen() const {
	return (listen_);
}
	
const std::vector<std::string>&  ServerConfig::getServerName() const {
	return (server_name_);
}

const std::map<int, std::string>& ServerConfig::getErrorPage() const {
	return (error_page_);
}

const std::string& ServerConfig::getRoot() const {
	return (root_);
}

size_t ServerConfig::getClientMaxBodySize() const {
	return (client_max_body_size_);
}

const std::vector<std::string>& ServerConfig::getIndex() const {
	return (index_);
}

const std::pair<int, std::string>& ServerConfig::getReturn() const {
	return (return_);
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
	return (locations_);
}

bool ServerConfig::getAutoIndex() const {
	return (autoindex_);
}

LocationConfig&	ServerConfig::getLastLocation() {
	if (locations_.empty()) {
		throw std::runtime_error("Tentative d'accès à une location dans un serveur vide !");
	}
	return (locations_.back());
}

const std::set<std::string>&	ServerConfig::getAllowedMethods() const {
	return (allowed_methods_);
}

const std::string&	ServerConfig::getUploadPath() const {
	return (upload_path_);
}

bool	ServerConfig::getAllowedUpload() const {
	return (allowed_upload_);
}

void	ServerConfig::setRoot(const std::string& str) {
	this->root_ = str;
}

void	ServerConfig::setIndex(const std::vector<std::string>& index) {
	this->index_ = index;
}

void	ServerConfig::setAutoIndex(bool allow) {
	autoindex_ = allow;
}

void	ServerConfig::setClientMaxBodySize(size_t size) {
	this->client_max_body_size_ = size;
}

void	ServerConfig::setServerName(const std::vector<std::string>& names) {
	server_name_ = names;
}

void	ServerConfig::setReturn(int code, const std::string& url) {
	return_.first = code;
	return_.second = url;
}

void	ServerConfig::setAllowedMethods(const std::set<std::string>& methods) {
	allowed_methods_ = methods;
}

void	ServerConfig::setUploadPath(const std::string& upload_path) {
	upload_path_ = upload_path;
}

void	ServerConfig::setAllowedUpload(bool allow) {
	allowed_upload_ = allow;
}

void	ServerConfig::addLocation(const LocationConfig& loc) {
	locations_.push_back(loc);
}

void	ServerConfig::addListen(const std::string& ip, int port) {
	Listen newlisten;
	newlisten.ip = ip;
	newlisten.port = port;
	this->listen_.push_back(newlisten);
}

void	ServerConfig::addErrorPage(int code, const std::string& path) {
	error_page_[code] = path;
}

void ServerConfig::finalize() {
	// 1. D'abord, on fixe les défauts du serveur s'il est vide
	if (this->root_.empty())
		this->root_ = "/var/www/html";
	// if (this->autoindex_ == -1)
		// this->autoindex_ = 0; // off
	// if (this->client_max_body_size_ == -1)
		// this->client_max_body_size_ = 1000000; // 1Mo
	if (this->allowed_methods_.empty()) {
		this->allowed_methods_.insert("GET"); // Défaut minimal
	}

	// 2. Ensuite, on propage vers chaque location
	for (size_t i = 0; i < locations_.size(); i++) {
		LocationConfig &loc = locations_[i];

		// Si la location n'a pas de root, elle prend celui du serveur
		if (loc.getRoot().empty())
			loc.setRoot(this->root_);

		// // Héritage de l'autoindex
		// if (loc.getAutoIndex() == -1)
		// 	loc.setAutoIndex(this->autoindex_);

		// // Héritage des méthodes autorisées
		// if (loc.getAllowedMethods().empty())
		// 	loc.setAllowedMethods(this->allowed_methods_);

		// // Héritage du dossier d'upload
		// if (loc.getUploadPath().empty())
		// 	loc.setUploadPath(this->upload_path_);
		
		// // Héritage du client_max_body_size
		// if (loc.getClientMaxBodySize() == -1)
		// 	loc.setClientMaxBodySize(this->client_max_body_size_);
	}
}

std::ostream& operator<<(std::ostream &stream, const ServerConfig& srv) {
	if (!srv.getListen().empty()) {
		stream << "Listen : ";
		for (size_t i = 0; i < srv.getListen().size(); i++)
		{
			if (!srv.getListen()[i].ip.empty())
				stream << "IP:" << srv.getListen()[i].ip << " ";
			if (srv.getListen()[i].port >= 0 && srv.getListen()[i].port < 65536)
				stream << "Port:" << srv.getListen()[i].port;
			std::cout << " | ";
		}
		stream << std::endl;
	}

	if (!srv.getServerName().empty()) {
		stream << "Server name: ";
		for (size_t i = 0; i < srv.getServerName().size() ; i++)
		{
			stream << srv.getServerName()[i] << " ";
		}
		stream << std::endl;
	}

	stream << "Error pages: ";
	std::map<int, std::string>::const_iterator it;
	for (it = srv.getErrorPage().begin(); it != srv.getErrorPage().end(); it++) {
		stream << "Error:" << it->first << " Page:" << it->second << "  |  ";
	}
	stream << std::endl;

	if (srv.getReturn().first != 0) { // On vérifie si un code est défini
		stream << "Return: " << srv.getReturn().first;
		if (!srv.getReturn().second.empty()) {
			stream << " (" << srv.getReturn().second << ")";
		}
		stream << std::endl;
	}

	stream << "Auto index: " << srv.getAutoIndex() << std::endl;
	stream << "Client max body size: " << srv.getClientMaxBodySize() << std::endl;
	if (!srv.getRoot().empty())
		stream << "Root: " << srv.getRoot() << std::endl;
	if (!srv.getIndex().empty()) {
		stream << "Index: ";
		for (size_t i = 0; i < srv.getIndex().size(); i++)
		{
			stream << srv.getIndex()[i] << " ";
		}
		
	}

	if (!srv.getAllowedMethods().empty()) {
		stream << "Allowed methods: ";

		std::set<std::string>::const_iterator it;
		for (it = srv.getAllowedMethods().begin(); it != srv.getAllowedMethods().end(); it++)
		{
			stream << *it << " ";
		}
		std::cout << std::endl;
	}

	if (!srv.getLocations().empty()) {
		stream << std::endl;
		stream << std::endl;
		stream << "Location: ";
		for (size_t i = 0; i < srv.getLocations().size(); i++)
		{
			stream << srv.getLocations()[i] << " ";
		}
	}
	
	
	return (stream);
}