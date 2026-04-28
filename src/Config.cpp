#include "Config.hpp"
#include <iostream>

std::vector<Location> Config::location_;
size_t Config::body_size_ = 450;

// Test Julien
//std::string	Config::root_ = "/home/julien/Webserv";
std::string	Config::root_ = "/home/juduchar/Common/Webserv";

// Test Romane
//std::string Config::root_ = "/home/rmetge/cursus/github/webserv";

Config::Config()
{
	std::vector<std::string> methods;
	methods.push_back("GET");

	std::vector<std::string> index;
	index.push_back("index.html");

	std::vector<std::string>cgiIndex;
	cgiIndex.push_back("index.php");

	Location cgiLoc("/cgi-bin", "./src/www", "/usr/bin/php-cgi", index, false, methods);
	
	cgiLoc.addCgiHandler(".php", "/usr/bin/php-cgi");

	location_.pushback(cgiLoc);

	Location rootLoc("/", "./src/www", "", index, false, methods);

	location_.push_back(rootLoc);

	//"/downloads", "./data", "./data/tmp", "secret_list.html", true

	// Test Romane
    // C'est ici que le push_back est autorisé
	/*
	std::vector<std::string> methods;
    methods.push_back("GET");
    //methods.push_back("POST");
	std::vector<std::string> index;
	index.push_back("/index.html");
	Location loc1("Makefile", "./data", "./data/tmp", index, true, methods);
	std::vector<std::string> method;
    method.push_back("DELETE");
	std::vector<std::string> index2;
	index2.push_back("test.html");
	Location loc2("/upload", "./src", "./src/tmp", index2, false, method);
	location_.push_back(loc2);
	*/
	// Fin test Romane

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
//{r); // O
//	return autoindex_;
//}


void Config::setBodySize(size_t value)
{
	(void)value;
	body_size_ = 450;
}

size_t	Config::getBodySize()
{
	return body_size_;
}

void Config::setRoot(size_t value)
{
	(void)value;
	//root_ = "/home/rmetge/cursus/github/webserv";
	root_ = "/home/juduchar/Common/Webserv";
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

//implemente des location, (test)
void	Config::location()
{
	std::vector<std::string> methods;
	methods.push_back("GET");

	std::vector<std::string> index;
	index.push_back("index.html");

	std::vector<std::string>cgiIndex;
	cgiIndex.push_back("index.php");

	Location cgiLoc("/cgi-bin", "./src/www", "/usr/bin/php-cgi", index, false, methods);
	
	cgiLoc.addCgiHandler(".php", "/usr/bin/php-cgi");

	location_.pushback(cgiLoc);

	Location rootLoc("/", "./src/www", "", index, false, methods);

	location_.push_back(rootLoc);

	/*
	std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
	std::vector<std::string> index;
	index.push_back("index.html");
	Location loc1("/src", "/home/juduchar/Common/Webserv", "./data/tmp", index, false, methods);
	//Location loc1("/src", "/home/rmetge/cursus/github/webserv", "./data/tmp", index, false, methods);
    location_.push_back(loc1);
	std::vector<std::string> method;
    method.push_back("POST");
	std::vector<std::string> index2;
	index2.push_back("test.html");
	Location loc2("/uploads", "/home/juduchar/Common/Webserv", "/uploads", index2, false, method);
	//Location loc2("/uploads", "/home/rmetge/cursus/github/webserv", "/uploads", index2, false, method);
	location_.push_back(loc2);
	std::vector<std::string> methode;
    methode.push_back("GET");
    methode.push_back("POST");
	//std::vector<std::string> index3;
	index.push_back("index.html");
	Location loc3("/Makefile", "/home/juduchar/Common/Webserv", "./data/tmp", index, false, methode);
	//Location loc3("/Makefile", "/home/rmetge/cursus/github/webserv", "./data/tmp", index, false, methode);
    location_.push_back(loc3);

	std::vector<std::string> methode2;
    methode2.push_back("DELETE");
    methode2.push_back("POST");
	//std::vector<std::string> index4;
	index.push_back("index.html");
	Location loc4("/obj", "/home/juduchar/Common/Webserv", "./data/tmp", index, false, methode2);
	//Location loc4("/obj", "/home/rmetge/cursus/github/webserv", "./data/tmp", index, false, methode2);
    location_.push_back(loc4);

	// on crée un vecteur pour lister les méthodes HTTP acceptées sur cette route
	std::vector<std::string> cgi_methods;
	// on autorise uniquement les requêtes GET (pour l'instant)
	cgi_methods.push_back("GET");

	// on crée un vecteur pour les fichiers à chercher si l'utilisateur demande le dossier cgi-bin
	std::vector<std::string> cgi_index;
	// si le client demande "http://localhost/cgi-bin/", le serveur cherchera le fichier test.php pour l'exécuter
	cgi_index.push_back("test.php");

	// on crée l'objet Location
	// Paramètres :
		// path : "/cgi-bin" : l'url tapée par le client (traduit en http://localhost/cgi-bin/)
		// root : "/home/julien/Webserv/cgi-bin" : le chemin absolu du dossier cgi-bin sur le PC
		// upload_path : "./data/tmp" : le dossier où stocker les uploads ou les fichiers temporaires
		// index : "cgi_index" (voir plus haut) la liste de fichiers index par défaut (index.php)
		// autoindex : "false" : l'auto-index est désactiver (indispensable pour un dossier CGI)
		// allowed_methods : "cgi_methods" (voir plus haut) : liste des méthodes autorisées (GET)
	
	// on ajoute cette route à la liste des locations gérées par la classe Config
	// matchLocation ira fouiller dans ce tableau
	Location	locCgi("/cgi-bin", "/home/julien/Webserv", "./data/tmp", cgi_index, false, cgi_methods);

	locCgi.addCgiHandler(".php", "/usr/bin/php-cgi");

	location_.push_back(locCgi);
	*/
}


//cherche la location par raport au path 
Location* Config::matchLocation(std::string requestPath) 
{
    Location* bestMatch = NULL;
    size_t longestLen = 0;
    std::vector<Location>::iterator it;

	std::cout << "Request Path " << requestPath << std::endl;
	std::cout << "Location size " << 	location_.size() << std::endl;

	

    for (it = location_.begin(); it != location_.end(); ++it)
    {
        std::string locPath = it->getPath();
        
		std::cout << "locPath : " << locPath << std::endl;
	
        if (requestPath.find(locPath) == 0) 
        {
			std::cout << "locPath : " << locPath.length() << std::endl;
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