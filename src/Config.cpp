#include "Config.hpp"

std::vector<Location> Config::location_;

Config::Config()
{
	//"/downloads", "./data", "./data/tmp", "secret_list.html", true

    // C'est ici que le push_back est autorisé
	std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
	std::vector<std::string> index;
	index.push_back("/index.html");
	Location loc1("/downloads", "./data", "./data/tmp", index, true, methods);
	std::vector<std::string> method;
    method.push_back("DELETE");
	std::vector<std::string> index2;
	index2.push_back("test.html");
	Location loc2("/upload", "./src", "./src/tmp", index2, false, method);
	location_.push_back(loc2);

	//std::vector<std::string> methodsDef;
    //methodsDef.push_back("GET");
	//std::vector<std::string> index3;
	//index3.push_back("test.html");
	//Location defaultLoc("/", "./www", "", index3, false, methodsDef);
	//defaultLocation_ = defaultLoc;

}
Config::~Config()
{}

//bool Config::autoindex_ = false;

//void Config::setAutoindex(bool value)
//{
//	(void)value;
//	autoindex_ = true;
//}

//bool	Config::getAutoindex()
//{
//	return autoindex_;
//}

size_t Config::body_size_ = 400;

void Config::setBodySize(size_t value)
{
	(void)value;
	body_size_ = 400;
}

size_t	Config::getBodySize()
{
	return body_size_;
}

std::string Config::root_ = "/home/rmetge/cursus/github/webserv";

void Config::setRoot(size_t value)
{
	(void)value;
	root_ = "/home/rmetge/cursus/github/webserv";
}

std::string	Config::getRoot()
{
	return root_;
}

//std::string Config::index_ = "/index.html"; // rejoute un / devant pour que je puis direct l'utiliser

//std::string	Config::getIndex()
//{
//	return index_;
//}


// Dans ta classe Config ou Server
Location* Config::matchLocation(std::string requestPath) 
{
    std::vector<Location>::iterator it;
    
    for (it = location_.begin(); it != location_.end(); ++it)
	{
        if (it->getPath() == requestPath)
            return &(*it); // On a trouvé la bonne config !
    }
    // Si rien n'est trouvé, on retourne une location par défaut ou on gère l'erreur
    return NULL;
	//throw 
}