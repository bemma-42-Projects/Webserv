#include "RequestAnswer.hpp"
#include <iostream>
#include <fcntl.h>    // pour open
#include <unistd.h>   // pour read, close
#include <sys/stat.h> // pour stat
#include "Config.hpp"
#include <dirent.h>
#include <sstream>

RequestAnswer::RequestAnswer()
{}

RequestAnswer::~RequestAnswer()
{}

std::string itoa(int nbr)
{
	std::stringstream ss;
    
    ss << nbr;
    std::string str = ss.str();
	return str;
}

std::string	RequestAnswer::getIfFile(std::string file)
{
	std::cout << Config::getRoot() + file << std::endl;
	int	fd = open((Config::getRoot() + file).c_str(), O_RDONLY);
	if (fd == -1)
		return "";
	std::string	res;
	char buffer[2000];//taille de la reponse ([4096])
	ssize_t	bite_read;
	while ((bite_read = read(fd, buffer, sizeof(buffer))) > 0)
	{
		res.append(buffer, bite_read);
	}
	close(fd);
	//std::cout << res << std::endl;
	return res; 
}

std::string	RequestAnswer::getIfDir(Request &request)
{
	//std::cout << "pd" << std::endl;
	DIR* dir = opendir(request.getPath().c_str());
	if (!dir)
	{
		std::cout << "error 404" << std::endl;
		return "";// Erreur 403 ou 404
	} 
	std::string	res;

	std::string body = "<html><head><title>Index of " + request.getUrlPath() + "</title></head><body>";
	body += "<h1>Index of " + request.getUrlPath() + "</h1><hr><ul>";
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) // reccupere fichier par fichier
	{
		std::string name = entry->d_name; // recupere le nom du fichier
		if (name == ".") // on ne dois pas annaliser le "." sinon on ouvre le dossier actuel et il faut qu'on le gere
			continue;
		// On construit le chemin complet pour que stat puisse le trouver
		std::string fullPath = request.getPath() + "/" + name;
		struct stat st;
		
		//std::string displayName = name;
		if (stat(fullPath.c_str(), &st) == 0) // regarde si le fichier existe
		{
			if (S_ISDIR(st.st_mode))
				name += "/"; // On ajoute un slash visuel
		}
		else
		{
			std::cout << "error 400" << std::endl;
			return "";
		} 
		// Le lien href doit être le nom, mais le texte affiché est displayName
		body += "<li><a href=\"" + name + "\">" + name + "</a></li>\n";
	}
	body += "</ul><hr></body></html>";
	closedir(dir);
	std::string header = "HTTP/1.1 200 OK\r\n";
	header += "Content-Type: text/html\r\n";
	header += "Content-Length: " + itoa(body.length()) + "\r\n"; // Il faudra une petite fonction pour convertir int en string
	header += "\r\n"; // La ligne vide cruciale !
	res = header + body;
	//std::cout << res << std::endl;
	return (res);
}


std::string	RequestAnswer::methodGet(Request &request)
{
	struct stat info;
	if (stat(request.getPath().c_str(), &info) != 0)
	{
		std::cerr << "error 404" << std::endl;
		return "";
	}
	std::string res;
	if (S_ISREG(info.st_mode))
		return (getIfFile(request.getUrlPath()));

	else if (S_ISDIR(info.st_mode))
		{
			//divier la fontion
			//!!!Le chemin relatif à la racine de ton serveur (l'URL). Si ton dossier webserv est la racine, l'utilisateur devrait juste voir Index of /.
			if (Config::getAutoindex() == true)
				return (getIfDir(request));
			else
			{
				return (getIfFile(Config::getIndex()));	
					//Sinon, renvoie la page par défaut (ex: index.html).
			}
		}
	return res;
}

std::string	RequestAnswer::answer(Request &request)
{
	//struct stat info;
	//if (stat(path_.c_str(), &info) != 0)
	//{
	//	std::cerr << "error 404" << std::endl;
	//	return "";
	//}
	//std::string res;
	if (request.getMethod() == "GET")
	{
		return (methodGet(request));
		//if (file .html .jpg)
		//if (S_ISREG(info.st_mode))
		//lire le fichier et renvoyer son contenu
		//if path est un repertoir regarde si autoindex est activé dans ta config.
		//else if (S_ISDIR(info.st_mode))
		//}
		//Si oui, génère une liste HTML des fichiers. 
		//Sinon, renvoie la page par défaut (ex: index.html).

	}

	else if (request.getMethod() == "DELETE")
	{
		if (unlink(request.getPath().c_str()) == 0)
			return "";
		else 
		{
			std::cout << "error 404 error supression"  << std::endl;
			return "";
		}
		//Utilise unlink() pour supprimer le fichier
	}
	//else if (method_ == "POST")
	//{
	//	//Si l'extension correspond à un script (ex: .php), tu dois préparer l'environnement (setenv) et fork() pour exécuter le CGI.
	//}
	////mettre le reponse dans une string et a renvoyer
	return "";
}