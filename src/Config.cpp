#include "Config.hpp"
#include <iostream>

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
	Location loc1("Makefile", "./data", "./data/tmp", index, true, methods);
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


//void	Config::location()
//{
//	std::vector<std::string> methods;
//    methods.push_back("GET");
//    methods.push_back("POST");
//	std::vector<std::string> index;
//	index.push_back("/index.html");
//	Location loc1("/Makefile", "./data", "./data/tmp", index, true, methods);
//	std::vector<std::string> method;
//    method.push_back("DELETE");
//	std::vector<std::string> index2;
//	index2.push_back("test.html");
//	Location loc2("/upload", "./src", "./src/tmp", index2, false, method);
//	location_.push_back(loc2);
//}


void Config::location()
{
    // --- LOCATION 1 : /Makefile ---
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
    std::vector<std::string> index;
    index.push_back("index.html"); // Pas de '/' devant l'index, c'est un nom de fichier

    Location loc1("/Makefile", "./data", "./data/tmp", index, true, methods);
    location_.push_back(loc1); // <--- IL MANQUAIT CETTE LIGNE

    // --- LOCATION 2 : /upload ---
    std::vector<std::string> method;
    method.push_back("DELETE");
    std::vector<std::string> index2;
    index2.push_back("test.html");

    Location loc2("/upload", "./src", "./src/tmp", index2, false, method);
    location_.push_back(loc2);
}

//// Dans ta classe Config ou Server
//Location* Config::matchLocation(std::string requestPath) 
//{
//    std::vector<Location>::iterator it;
//    std::cout << "DEBUG: Size of location_ = " << location_.size() << std::endl;
//    for (it = location_.begin(); it != location_.end(); ++it)
//	{
//		std::cout << it->getPath() << std::endl;
//        if (it->getPath() == requestPath)
//            return &(*it); // On a trouvé la bonne config !
//    }
//    // Si rien n'est trouvé, on retourne une location par défaut ou on gère l'erreur
//    return NULL;
//	//throw 
//}

Location* Config::matchLocation(std::string requestPath) 
{
    std::cout << "--- START MATCHING FOR: '" << requestPath << "' ---" << std::endl;
    std::cout << "Entries in location_ vector: " << location_.size() << std::endl;

    Location* bestMatch = NULL;
    size_t longestLen = 0;

    for (std::vector<Location>::iterator it = location_.begin(); it != location_.end(); ++it)
    {
        std::string locPath = it->getPath();
        std::cout << "  Checking against: '" << locPath << "'" << std::endl;
        
        if (requestPath.find(locPath) == 0) 
        {
            if (locPath.length() > longestLen) 
            {
                longestLen = locPath.length();
                bestMatch = &(*it);
                std::cout << "  -> Potential match found!" << std::endl;
            }
        }
    }
    if (!bestMatch)
        std::cout << "  -> NO MATCH FOUND (Return NULL)" << std::endl;
    return bestMatch;
}

//le math avec l'url n'est pas bon
//voir la fonction match et l'implementation de location