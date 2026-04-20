#include "RequestAnswer.hpp"
#include "CGISubprocess.hpp"

#include <iostream>
#include <fcntl.h>		// pour open
#include <unistd.h>		// pour read, close, fork et execve
#include <sys/stat.h>	// pour stat
#include <sys/wait.h>	// pour waitpid
//#include <cstdlib>		// pour exit
#include "Config.hpp"
#include <dirent.h>
#include <sstream>
#include <map>
#include <cstring>	// pour strcpy

//initialise les variable
RequestAnswer::RequestAnswer(Request &request) : request_(request)
{
	//request_ = request;
	answer_ = "";
	error_ = 0;
	this->cgi_process_ = NULL;
}

RequestAnswer::~RequestAnswer()
{
	if (this->cgi_process_ != NULL)
	{
		delete (this->cgi_process_);
		this->cgi_process_ = NULL;
	}
}

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

//trouve le content_type_
std::string RequestAnswer::findContentType(const std::string& path) 
{
    static std::map<std::string, std::string> mimeTypes;

    // Initialisation au premier appel (static)
    if (mimeTypes.empty()) {
        // TEXTE
        mimeTypes[".html"] = "text/html";
        mimeTypes[".htm"]  = "text/html";
        mimeTypes[".css"]  = "text/css";
        mimeTypes[".txt"]  = "text/plain";
        mimeTypes[".cpp"]  = "text/plain"; // Pour tes fichiers source
        mimeTypes[".hpp"]  = "text/plain";

        // IMAGES
        mimeTypes[".png"]  = "image/png";
        mimeTypes[".jpg"]  = "image/jpeg";
        mimeTypes[".jpeg"] = "image/jpeg";
        mimeTypes[".gif"]  = "image/gif";
        mimeTypes[".ico"]  = "image/x-icon";

        // APPLICATION / BINAIRE
        mimeTypes[".js"]   = "application/javascript";
        mimeTypes[".json"] = "application/json";
        mimeTypes[".pdf"]  = "application/pdf";
        mimeTypes[".zip"]  = "application/zip";
    }

    // Trouver l'extension (tout ce qui est après le dernier point)
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) 
		return "application/octet-stream";
	std::string ext = path.substr(dotPos);
	for (size_t i = 0; i < ext.length(); ++i) 
		ext[i] = std::tolower(ext[i]);
	if (mimeTypes.count(ext))
		return mimeTypes[ext];
	return "application/octet-stream";

    // Type par défaut si l'extension est inconnue ou absente
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
	body_ = res;
	code_ = 200;
	content_type_ = findContentType(file);
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
		code_ = 404;
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
			code_ = 400;
			return 1;
		} 
		// Le lien href doit être le nom, mais le texte affiché est displayName
		body += "<li><a href=\"" + name + "\">" + name + "</a></li>\n";
	}
	body += "</ul><hr></body></html>";
	closedir(dir);
	//std::string header = "HTTP/1.1 200 OK\r\n";
	//header += "Content-Type: text/html\r\n";
	//header += "Content-Length: " + itoa(body.length()) + "\r\n"; // Il faudra une petite fonction pour convertir int en string
	//header += "\r\n"; // La ligne vide cruciale !
	//res = header + body;
	//std::cout << res << std::endl;

	//answer_ = res;
	body_ = body;
	code_ = 200;
	content_type_ = "text/html";
	return 0;
}

//cherche un index qui existe et est lisible et on le renvoi
std::string RequestAnswer::findIndex(Location loc)
{
    std::vector<std::string>::iterator it;
	std::vector<std::string> index = loc.getIndex();
    for (it = index.begin(); it != index.end(); ++it)
	{
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
//int	RequestAnswer::setAnswer()
int	RequestAnswer::methodGet()
{
	struct stat info;
	if (stat(request_.getPath().c_str(), &info) != 0)
	{
		std::cerr << "error  404" << std::endl;
		error_ = 404;
		code_ = 404;
		return 1;
	}
	std::string res;
	// si c'est un REGULAR FILE
	if (S_ISREG(info.st_mode))
	{
		// si c'est un CGI
		if (this->isCgi())
			// on exécute la méthode CGI
			return (this->methodCGI());
		return (getIfFile(request_.getPath()));
	}
	// si c'est un DIRECTORY
	// ...
	else if (S_ISDIR(info.st_mode))
	{
		Location	loc = request_.getLocation();
		//divier la fontion
		//!!!Le chemin relatif à la racine de ton serveur (l'URL). Si ton dossier webserv est la racine, l'utilisateur devrait juste voir Index of /.
		std::cout << "dir" << std::endl;
		std::string index = findIndex(loc);
		std::cout << "dir" << std::endl;
		if (!index.empty())
		{
			return (getIfFile(Config::getRoot() + '/' + index));	
			//Sinon, renvoie la page par défaut (ex: index.html).
		}
		else if (loc.getAutoindex() == true)
			return (getIfDir());
		else
		{
			error_ = 403;
			code_ = 403;
		}
	}
	return 1;
}

//upload les fichier
//int	RequestAnswer::setAnswer()
/*
int	RequestAnswer::methodPost()
{
	struct stat s;
	if (stat(request_.getPath().c_str(), &s) != 0)
	{
		//trouve le nom du fichier
		int	id = request_.getBody().find("filename=");
		int end = request_.getBody().find("filename=");
		std::string	filename = 
	}
	
}
*/

//faire la reponse avec le header
//int	RequestAnswer::setAnswer()
void	RequestAnswer::fullAnswer()
{
	std::string header = request_.getVersion() + ' ' + itoa(code_);
	if (code_ == 200)
		header += " OK\r\n";
	else if (code_ == 201)
		header += " Created\r\n";
	else if (code_ == 301)
		header += " Moved\r\n";
	else
	{
		header += " Not Found\r\n";
		content_type_ = "text/html";
	}
	header += "Content-Type: " + content_type_ + "\r\n";
	header += "Content-Length: " + itoa(body_.length()) + "\r\n";
	header += "\r\n\r\n";

	//std::cout << "header = " << header << std::endl;

	answer_ = header + body_;
}

//envoie les fonction par rapport au methode (get, post, delete)
int	RequestAnswer::setAnswer()
{
	answer_.clear();
	if (request_.getMethod() == "GET")
	{
		methodGet();
		//if (methodGet() != 0)
			//return 0;//error
		//else
		//	return 1;//get
	}

	else if (request_.getMethod() == "DELETE")
	{
		if (unlink(request_.getPath().c_str()) != 0)
		{
			std::cout << "error 404 error supression"  << std::endl;
			error_ = 404;
			code_ = 404;
			//return (0);//error
		}
		//else 
		//	return (2);//delete
		//Utilise unlink() pour supprimer le fichier
	}
	else if (request_.getMethod() == "POST")
	{
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
		//}
	}
////mettre le reponse dans une answer_
	fullAnswer();
	//std::cout << body_ << std::endl;
	return 1;
}

//upload 244 recuper le nom du fichier dans le body

// fonction pour déterminer si c'est un cgi
// et pour stocker l'interpreter correspondant


bool	RequestAnswer::isCgi()
{
	std::map<std::string, std::string>	cgi_handlers = request_.getLocation().getCgiHandlers();

	std::string	url = request_.getUrlPath();

	size_t	last_point_position = url.find_last_of(".");
	
	if (last_point_position == std::string::npos)
		return (false);

	std::string	extension = url.substr(last_point_position);

	std::map<std::string, std::string>::iterator it = cgi_handlers.find(extension);

	if (it != cgi_handlers.end())
	{
		// DEBUG
		std::cout << "extension : " << it->first << std::endl;
		std::cout << "interpreter : " << it->second << std::endl;
		//
		this->cgi_interpreter_ = it->second;
		return (true);
	}
	return (false);
}

// fonction pour exécuter le CGI
// on est ici si c'est un CGI
int	RequestAnswer::methodCGI()
{
	char	**envp = this->getEnvp();

	try {
		//CGISubprocess cgi_process;
        
		this->cgi_process_ = new CGISubprocess();

		this->cgi_process_->createSubprocess(this->request_.getPath(), this->cgi_interpreter_, envp);
		
		if (this->request_.getMethod() == "GET")
			close(this->cgi_process_->getWriteFd());
    } catch (const std::exception& e) {
        std::cerr << "CGI Error : " << e.what() << std::endl;
		this->error_ = 500;
    }

	int i = 0;
	while (envp[i] != NULL)
	{
		delete[] envp[i];
		i++;
	}
	delete[] envp;

	return (1);
}

// en CGI, le seul moyen de communication entre le serveur
// et le script PHP (avant son exécution)
// sont les variables d'environnement
char		**RequestAnswer::getEnvp()
{
	std::vector<std::string>	env;

	// la version du protocole CGI utilisée par le serveur
	// CGI/1.1
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");

	// la version du protocole HTTP utilisée par la requête
	// HTTP/1.1
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");

	// la méthode utilisée par le client
	// exemples : GET ou POST ou DELETE
	env.push_back("REQUEST_METHOD=" + request_.getMethod());
	

	// l'URL complète demandée par le client (path + paramètres)
	// exemple : /cgi-bin/script.php?user=test
	env.push_back("REQUEST_URI=" + request_.getRequestUri());
	
	// le chemin absolu vers le script
	// exemple : /home/julien/Webserv/cgi-bin/script.php
	env.push_back("SCRIPT_FILENAME=" + request_.getPath());
	
	// le chemin virtuel vers le script (l'URI sans la query string)
	// exemple : /cgi-bin/script.php
	env.push_back("SCRIPT_NAME=" + request_.getUrlPath());

	// tout ce qui se trouve après le "?" dans l'URL (query string)
	// exemple : user=test&age=25 
	env.push_back("QUERY_STRING=" + request_.getQueryString());
	
	// le type de contenu envoyé dans le body
	// indispensable pour les formulaires POST
	// exemple : application/x-www-form-urlencoded
	env.push_back("CONTENT_TYPE=" + request_.getContentType());
	
	// la taille en octets des données du body
	// indispensable pour que PHP lise le POST
	// exemple : 15
	std::stringstream	ss_len;
	ss_len << request_.getContentLength();
	env.push_back("CONTENT_LENGTH=" + ss_len.str());
	
	// le nom de domaine du serveur
	// ou l'adresse IP utilisée pour l'atteindre
	// exemples : localhost ou 127.0.0.1
	env.push_back("SERVER_NAME=" + request_.getHost());
	
	// le port sur lequel le serveur écoute
	// exemple : 8080
	std::stringstream	ss_port;
	ss_port << request_.getPort();
	env.push_back("SERVER_PORT=" + ss_port.str());
	
	// l'adresse IP du client qui a fait la requête
	// exemple : 192.168.1.10
	env.push_back("REMOTE_ADDR=" + request_.getClientIP());
	
	// code de statut de redirection
	// exigé par php-cgi pour s'exécuter (mesure de sécurité)
	// 200
	env.push_back("REDIRECT_STATUS=200");

	this->addHeadersToEnv(env);

	char			**envp = new char*[env.size() + 1];
	std::size_t		i = 0;

	while (i < env.size())
	{
		envp[i] = new char[env[i].length() + 1];
		strcpy(envp[i], env[i].c_str());
		i++;
	}
	envp[env.size()] = NULL;

	// --- DEBUG ---
    std::cout << "\n=== DEBUG CGI ENV ===" << std::endl;
    for (int j = 0; envp[j] != NULL; j++)
    {
        std::cout << "envp[" << j << "] : " << envp[j] << std::endl;
    }
    std::cout << "=====================\n" << std::endl;
    // ------------------------------------------------------
	return (envp);
}

// cette fonction convertit les headers envoyés par le client
// en variables d'environnement
// pour le CGI
	// ajoute le préfixe HTTP_
	// convertit le nom en majuscules
	// remplace "-" par "_"
void	RequestAnswer::addHeadersToEnv(std::vector<std::string>& env_vector)
{
	std::map<std::string, std::string>	headers = request_.getHeaders();

	std::map<std::string, std::string>::iterator	it = headers.begin();

	while (it != headers.end())
	{
		std::string	key = it->first;
		std::string	value = it->second;

		if (key == "Content-Type" || key == "Content-Length")
		{
			++it;
			continue;
		}

		std::string	env_key = "HTTP_";

		size_t	i = 0;
		while (i < key.length())
		{
			if (key[i] == '-')
				env_key += '_';
			else
				env_key += toupper(key[i]);
			i++;
		}
		env_vector.push_back(env_key + "=" + value);
		++it;
	}
}
