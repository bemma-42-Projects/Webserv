#include "RequestAnswer.hpp"
#include <iostream>
#include <fcntl.h>    // pour open
#include <unistd.h>   // pour read, close
#include <sys/stat.h> // pour stat
#include "Config.hpp"
#include <dirent.h>
#include <sstream>

//initialise les variable
RequestAnswer::RequestAnswer(Request request)
{
	request_ = request;
	answer_ = "";
	error_ = 0;
}

RequestAnswer::~RequestAnswer()
{}

//return le reponse
std::string	RequestAnswer::getAnswer()
{
	return answer_;
}

//return l'error
int	RequestAnswer::getError()
{
	return error_;
}

std::string itoa(int nbr)
{
	std::stringstream ss;
    
    ss << nbr;
    std::string str = ss.str();
	return str;
}


//recupere le contenue du fichier pour la methode get
//int	RequestAnswer::getMethode()
int	RequestAnswer::getIfFile(std::string file)
{
	//std::cout << Config::getRoot() + file << std::endl;
	//int	fd = open((request_.getLocation().getRoot() + '/' + file).c_str(), O_RDONLY);
	std::cout << "dir" << std::endl;
	int	fd = open((file).c_str(), O_RDONLY);
	if (fd == -1)
		return 1;
	std::string	res;
	char buffer[2000];//taille de la reponse ([4096])
	ssize_t	bite_read;
	while ((bite_read = read(fd, buffer, sizeof(buffer))) > 0)
	{
		res.append(buffer, bite_read);
	}
	close(fd);
	//std::cout << res << std::endl;
	answer_ = res;
	return 0; 
}

//recupere le contenue du dossier pour la methode get
int	RequestAnswer::getIfDir()
{
	std::cout << "pd" << std::endl;
	DIR* dir = opendir(request_.getPath().c_str());
	if (!dir)
	{
		std::cout << "error 404" << std::endl;
		error_ = 404;
		return 1;// Erreur 403 ou 404
	} 
	std::string	res;

	std::string body = "<html><head><title>Index of " + request_.getUrlPath() + "</title></head><body>";
	body += "<h1>Index of " + request_.getUrlPath() + "</h1><hr><ul>";
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) // reccupere fichier par fichier
	{
		std::string name = entry->d_name; // recupere le nom du fichier
		if (name == ".") // on ne dois pas annaliser le "." sinon on ouvre le dossier actuel et il faut qu'on le gere
			continue;
		// On construit le chemin complet pour que stat puisse le trouver
		std::string fullPath = request_.getPath() + "/" + name;
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
			error_ = 400;
			return 1;
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
	answer_ = res;
	return 0;
}

//cherche un index qui existe et est lisible et on le renvoi
std::string RequestAnswer::findIndex(Location loc)
{
    std::vector<std::string>::iterator it;
    for (it = loc.getIndex().begin(); it != loc.getIndex().end(); ++it) {
		const std::string root = loc.getRoot();
		std::cout << "test1" << std::endl;
		std::cout << root << std::endl;
		std::cout << "it = " << *it << std::endl;
		std::string fullPath = root + '/' + *it;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		std::cout << "test2" << std::endl;
        
        // On utilise la fonction access() de <unistd.h> 
        // pour vérifier si le fichier existe et est lisible
        if (access(fullPath.c_str(), R_OK) == 0)
            return *it; // On a trouvé le premier index valide !
    }
    return ""; // Aucun index trouvé
}

//envoie les fonction pour la methode get (dossier ou fichier)
int	RequestAnswer::methodGet()
{
	struct stat info;
	if (stat(request_.getPath().c_str(), &info) != 0)
	{
		std::cerr << "error  404" << std::endl;
		error_ = 404;
		return 1;

	}
	std::string res;
	if (S_ISREG(info.st_mode))
		return (getIfFile(request_.getPath()));

	else if (S_ISDIR(info.st_mode))
	{
		Location	loc = request_.getLocation();
		//divier la fontion
		//!!!Le chemin relatif à la racine de ton serveur (l'URL). Si ton dossier webserv est la racine, l'utilisateur devrait juste voir Index of /.
		std::cout << "dir" << std::endl;
		std::string index = findIndex(loc);
		//if
		std::cout << "dir" << std::endl;
		if (loc.getAutoindex() == true)
			return (getIfDir());
		else if (!index.empty())
		{
			//return (index);
			
			return (getIfFile(Config::getRoot() + '/' + index));	
			//Sinon, renvoie la page par défaut (ex: index.html).
		}
		else
			error_ = 403;
	}
	return 1;
}

//envoie les fonction par rapport au methode (get, post, delete)
int	RequestAnswer::setAnswer()
{
	answer_.clear();
	if (request_.getMethod() == "GET")
	{
		if (methodGet() == 0)
			return 1;//get
		else
			return 0;//error
	}

	else if (request_.getMethod() == "DELETE")
	{
		if (unlink(request_.getPath().c_str()) == 0)
			return (2);//delete
		else 
		{
			std::cout << "error 404 error supression"  << std::endl;
			error_ = 404;
			return (0);//error
		}
		//Utilise unlink() pour supprimer le fichier
	}
	//else if (request_.getMethod() == "POST")
	//{
	//	std::string	url = request_.getUrlPath();
	//	if (/*(url.size() >= 4 && url.substr(url.size() - 4) == ".php")
	//		|| (url.size() >= 3 && (url.substr(url.size() - 3) == ".py")
	//		|| url.substr(url.size() - 3) == ".pl")*/)
	//	{

	//		//CGI
	//		//Si CGI → fork + pipe + execve avec body_ en entrée
	//	}
	//	else
	//	{
			
	//		//upload
	//		//Si upload → ouvrir un fichier sur path_ et y écrire body_
	//		//Si succès → construire une réponse 201 Created
	//		//Si échec → remplir error_ et retourner 0 comme tu fais déjà
	//	}
//}
////mettre le reponse dans une answer_
return 1;
}

//big probleme avec dir // ligne 127 pb => *it il veut pas donner la sting