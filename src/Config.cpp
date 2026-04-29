#include "Config.hpp"
#include <iostream>

std::vector<LocationConfig> Config::location_;

Config::Config()
{
	{
        LocationConfig loc;
        std::vector<std::string> methods;
        methods.push_back("DELETE");
        
        std::vector<std::string> index;
        index.push_back("test.html");

        loc.setPath("/upload");
        loc.setRootLoc("./src");
        loc.setUploadPath("./src/tmp");
        loc.setIndex(index);
        loc.setAutoIndex(false);
        loc.setAllowedMethods(methods);
        
        // On ajoute la location au vecteur statique de la classe
        location_.push_back(loc);
    }

    // --- Configuration de la Location par défaut (/) ---
    {
        LocationConfig defaultLoc;
        std::vector<std::string> methodsDef;
        methodsDef.push_back("GET");
        
        std::vector<std::string> indexDef;
        indexDef.push_back("test.html");

        defaultLoc.setPath("/");
        defaultLoc.setRootLoc("./www");
        defaultLoc.setUploadPath(""); // Vide si non utilisé
        defaultLoc.setIndex(indexDef);
        defaultLoc.setAutoIndex(false);
        defaultLoc.setAllowedMethods(methodsDef);

        // Si tu as une variable spécifique pour la location par défaut :
        //defaultLocation_ = defaultLoc;
        // Ou si elle va aussi dans le vecteur :
        // location_.push_back(defaultLoc);
    }
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
//{r); // O
//	return autoindex_;
//}

size_t Config::body_size_ = 450;

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

std::map<int, std::string> Config::error_;


std::map<int, std::string>	Config::getError()
{
    error_[404] = "./error.txt";
	return error_;
}

//std::vector<std::string> Config::index_ = "/index.html"; // rejoute un / devant pour que je puis direct l'utiliser

//std::vector<std::string>	Config::getIndex()
//{
//	return index_;
//}

//implemente des location, (test)
void	Config::location()
{
	//// --- LOCATION 1 : /src ---
    //{
    //    LocationConfig loc;
    //    std::vector<std::string> methods;
    //    methods.push_back("GET");
    //    methods.push_back("POST");

    //    std::vector<std::string> index;
    //    index.push_back("indexj.html");

    //    loc.setPath("/src");
    //    loc.setRootLoc("/home/rmetge/cursus/github/webserv");
    //    loc.setUploadPath("./data/tmp");
    //    loc.setIndex(index);
    //    loc.setAutoIndex(false);
    //    loc.setAllowedMethods(methods);

    //    location_.push_back(loc);
    //}

    // --- LOCATION 2 : /uploads ---
    {
        LocationConfig loc;
        std::vector<std::string> methods;
        methods.push_back("POST");

        std::vector<std::string> index;
        index.push_back("test.html");

        loc.setPath("/uploads");
        loc.setRootLoc("/home/rmetge/cursus/github/webserv");
        loc.setUploadPath("/uploads");
        loc.setIndex(index);
        loc.setAutoIndex(false);
        loc.setAllowedMethods(methods);

        location_.push_back(loc);
    }

    // --- LOCATION 3 : /Makefile ---
    {
        LocationConfig loc;
        std::vector<std::string> methods;
        methods.push_back("GET");
        methods.push_back("POST");

        std::vector<std::string> index;
        index.push_back("indexj.html"); // Attention: dans ton code original tu réutilisais l'ancien index
        index.push_back("index.html");

        loc.setPath("/Makefile");
        loc.setRootLoc("/home/rmetge/cursus/github/webserv");
        loc.setUploadPath("./data/tmp");
        loc.setIndex(index);
        loc.setAutoIndex(false);
        loc.setAllowedMethods(methods);

        location_.push_back(loc);
    }

    // --- LOCATION 4 : /obj ---
    {
        LocationConfig loc;
        std::vector<std::string> methods;
        methods.push_back("DELETE");
        methods.push_back("POST");

        std::vector<std::string> index;
        index.push_back("indexj.html"); 
        index.push_back("index.html");
        // index.push_back("index.html"); // Ajouté comme dans ton exemple

        loc.setPath("/src");
        loc.setRootLoc("/home/rmetge/cursus/github/webserv");
        loc.setUploadPath("./data/tmp");
        loc.setIndex(index);
        loc.setAutoIndex(false);
        loc.setAllowedMethods(methods);

        location_.push_back(loc);
    }
}


//cherche la location par raport au path 
//int	Request::parsingHttp()
LocationConfig* Config::matchLocation(std::string requestPath) 
{
    LocationConfig* bestMatch = NULL;
    size_t longestLen = 0;
    std::vector<LocationConfig>::iterator it;

    for (it = location_.begin(); it != location_.end(); ++it)
    {
        std::string locPath = it->getPath();
        
        if (requestPath.find(locPath) == 0) 
        {
            if (locPath.length() > longestLen) 
            {
                longestLen = locPath.length();
                bestMatch = &(*it);
            }
        }
    }
    return bestMatch;
}

//le math avec l'url n'est pas bon
//voir la fonction match et l'implementation de location 