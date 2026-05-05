#include "RequestAnswer.hpp"
#include "CGIHandler.hpp"
#include "CGISubprocess.hpp"

#include <iostream>
#include <fcntl.h>		// pour open
#include <unistd.h>		// pour read, close, fork et execve
#include <sys/stat.h>	// pour stat
#include <sys/wait.h>	// pour waitpid
//#include <cstdlib>	// pour exit
#include "Config.hpp"
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <map>
#include <cstring>	// pour strcpy

// constructeur par défaut
RequestAnswer::RequestAnswer() : code_(200), error_(0), request_(NULL), cgi_handler_(NULL)
{
	this->answer_ = "";
	this->content_type_ = "";
	this->body_ = "";
	this->post_file_name_ = "";
	this->cgi_interpreter_ = "";
}

// constructeur par copie
RequestAnswer::RequestAnswer(const RequestAnswer &src) : cgi_handler_(NULL)
{
	*this = src;
}

RequestAnswer	&RequestAnswer::operator=(const RequestAnswer &rhs)
{
	if (this != &rhs)
	{
		if (this->cgi_handler_ != NULL)
		{
			delete (this->cgi_handler_);
			this->cgi_handler_ = NULL;
		}
		this->code_ = rhs.code_;
		this->error_ = rhs.error_;
		this->request_ = rhs.request_;
		this->answer_ = rhs.answer_;
		this->content_type_ = rhs.content_type_;
		this->body_ = rhs.body_;
		this->post_file_name_ = rhs.post_file_name_;
		this->cgi_interpreter_ = rhs.cgi_interpreter_;
		this->cgi_handler_ = NULL;
	}
	return (*this);
}

// destructeur
RequestAnswer::~RequestAnswer()
{
	if (this->cgi_handler_ != NULL)
	{
		delete (this->cgi_handler_);
		this->cgi_handler_ = NULL;
	}
}

const std::string	&RequestAnswer::getAnswer() const
{
	return (this->answer_);
}

//return l'error
int	RequestAnswer::getError() const
{
	return (this->error_);
}

CGIHandler      *RequestAnswer::getCGIHandler() const
{
    return (cgi_handler_);
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
AnswerStatus	RequestAnswer::getIfFile(std::string file)
{
	//std::cout << Config::getRoot() + file << std::endl;
	//int	fd = open((request_->getLocation().getRoot() + '/' + file).c_str(), O_RDONLY);
	//std::cout << "dir" << std::endl;
	int	fd = open((file).c_str(), O_RDONLY);
	if (fd == -1)
	{
		return (ERROR);
	}
	std::string	res;
	char buffer[2000];
	ssize_t	bytes_read;
	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
	{
		res.append(buffer, bytes_read);
	}
	close(fd);
	//std::cout << res << std::endl;
	this->body_ = res;
	this->code_ = 200;
	this->content_type_ = findContentType(file);
	return (READY_TO_SEND);
}

//recupere le contenue du dossier pour la methode get
AnswerStatus	RequestAnswer::getIfDir()
{
	// std::cout << "pd" << std::endl;
	DIR* dir = opendir(request_->getPath().c_str());
	if (!dir)
	{
		// A verifier
		std::cout << "error 403" << std::endl;
		this->error_ = 403;
		return (ERROR);
	} 

	std::string body = "<html><head><title>Index of " + request_->getUrlPath() + "</title></head><body>";
	body += "<h1>Index of " + request_->getUrlPath() + "</h1><hr><ul>";
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) // reccupere fichier par fichier
	{
		std::string name = entry->d_name; // recupere le nom du fichier
		if (name == ".") // on ne dois pas annaliser le "." sinon on ouvre le dossier actuel et il faut qu'on le gere
			continue;
		// On construit le chemin complet pour que stat puisse le trouver
		std::string fullPath = request_->getPath() + "/" + name;
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
			this->error_ = 400;
			this->code_ = 400;
			return (ERROR);
		} 
		body += "<li><a href=\"" + name + "\">" + name + "</a></li>\n";
	}
	body += "</ul><hr></body></html>";
	closedir(dir);

	// A quoi sert ce bloc ?
	// std::string	res;
	//std::string header = "HTTP/1.1 200 OK\r\n";
	//header += "Content-Type: text/html\r\n";
	//header += "Content-Length: " + itoa(body.length()) + "\r\n"; // Il faudra une petite fonction pour convertir int en string
	//header += "\r\n"; // La ligne vide cruciale !
	//res = header + body;
	//std::cout << res << std::endl;

	//answer_ = res;
	this->body_ = body;
	this->code_ = 200;
	this->content_type_ = "text/html";
	// TODO : vérifier quand est appelé getIfDir et
	// checker READY_TO_SEND, plus 0
	return (READY_TO_SEND);
	//return 0;
}

//cherche un index qui existe et est lisible et on le renvoi
std::string RequestAnswer::findIndex(Location loc)
{
    std::vector<std::string>::iterator it;
	std::vector<std::string> index = loc.getIndex();
    for (it = index.begin(); it != index.end(); ++it)
	{
		const std::string root = loc.getRoot();
		// std::cout << "test1" << std::endl;
		// std::cout << root << std::endl;

		// std::cout << "it = " << *it << std::endl;

		std::string fullPath = root + '/' + *it;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		// std::cout << "test2" << std::endl;
        
        // On utilise la fonction access() de <unistd.h> 
        // pour vérifier si le fichier existe et est lisible
        if (access(fullPath.c_str(), R_OK) == 0)
            return *it; // On a trouvé le premier index valide !
    }
    return ""; // Aucun index trouvé
}

//envoie les fonction pour la methode get (dossier ou fichier)
//int	RequestAnswer::setAnswer()
AnswerStatus	RequestAnswer::methodGet()
{
	struct stat info;
	if (stat(request_->getPath().c_str(), &info) != 0)
	{
		std::cerr << "error 404" << std::endl;
		this->error_ = 404;
		this->code_ = 404;
		return (ERROR);
	}
	//std::string res;
	// si c'est un REGULAR FILE
	if (S_ISREG(info.st_mode))
	{
		// si c'est un CGI
		if (this->isCgi())

			return (this->methodCGI());
		return (getIfFile(request_->getPath()));
	}
	// si c'est un DIRECTORY
	// ...
	else if (S_ISDIR(info.st_mode))
	{
		Location	loc = request_->getLocation();
		//divier la fontion
		//!!!Le chemin relatif à la racine de ton serveur (l'URL). Si ton dossier webserv est la racine, l'utilisateur devrait juste voir Index of /.
		// std::cout << "dir" << std::endl;
		std::string index = findIndex(loc);
		if (!index.empty())
		{
			return (getIfFile(Config::getRoot() + '/' + index));
		}
		else if (loc.getAutoindex() == true)
			return (getIfDir());
		else
		{
			this->error_ = 403;
			this->code_ = 403;
		}
	}
	return (ERROR);
}

AnswerStatus	RequestAnswer::methodPost()
{
	Location	loc = request_->getLocation();

	if (this->isCgi())
    {
        std::cout << "[DEBUG] Detection CGI reussie, interpreteur : " << this->cgi_interpreter_ << std::endl;
        
        try {
            // Nettoyage de sécurité si un handler existait déjà
            if (this->cgi_handler_)
                delete this->cgi_handler_;

            // On utilise l'interpréteur trouvé par isCgi() !
            this->cgi_handler_ = new CGIHandler(*request_, this->cgi_interpreter_);
            this->cgi_handler_->execute();
            return (CGI_IN_PROGRESS);
        } catch (const std::exception& e) {
            std::cerr << "[CGI Error] " << e.what() << std::endl;
            this->error_ = 500;
            return (ERROR);
        }
    }

	std::string root = loc.getRoot() + loc.getPath();
	struct stat s;
	if (stat(root.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		//std::cout << "deb" << std::endl;
		std::cout << "[DEBUG] Tentative d'upload dans le dossier : " << root << std::endl;

		struct stat p;
		stat(request_->getPath().c_str(), &p);
		if (stat(request_->getPath().c_str(), &p) == 0 && S_ISREG(p.st_mode))
		{
			post_file_name_ = request_->getPath();
			//std::cout << post_file_name_ << std::endl;
			return (ERROR);
		}
		//bool	quote = false;
		std::string body = request_->getBody();
		size_t		id = body.find("Content-Disposition:");
		if (id == std::string::npos)
			return (READY_TO_SEND);
		size_t start = body.find("filename=", id);
		if (start == std::string::npos)
			return (READY_TO_SEND);
		start += 9;

		while (body[start] == ' ')
			++start;
		bool	quote = false;
		if (body[start] == '\"')
		{
			++start;
			quote = true;
		}
		size_t end = body.find("\r\n", start);
		if (end == std::string::npos)
			return (READY_TO_SEND);
		//while (quote == true)
		//{
		//	if (body[end - 1] == '\"')
		//		quote = false;
		//	--end;
		//}
		if (quote && body[end - 1] == '\"')
			--end;
		
		size_t	s = body.find_last_of('/', end);
		if (s != std::string::npos && s >= start)
			start = s + 1;
		std::string file_name = body.substr(start, end - start);
		//std::cout << "file name = " << file_name << std::endl;
		// struct stat f;
		std::string full_dest_path = root + '/' + file_name;
		
		struct stat b;
		if (stat(full_dest_path.c_str(), &b) == 0 && S_ISREG(b.st_mode))
		{
			post_file_name_ = full_dest_path;
			return (ERROR);
		}
	}
	std::cout << "error" << std::endl;
	return (READY_TO_SEND);
}

//recupere le path du file name pour upload les fichier
int RequestAnswer::fileName()
{
    Location    loc = this->request_->getLocation();
    std::string root_path = loc.getRoot() + loc.getPath(); // Chemin dossier sur disque
    std::string url_path = this->request_->getPath();           // Chemin demandé dans l'URL

    struct stat s;
    bool is_directory = false;

    // ÉTAPE 1 : On vérifie si l'URL pointe vers un dossier existant
    if (stat(url_path.c_str(), &s) == 0) {
        if (S_ISDIR(s.st_mode)) {
            is_directory = true;
        }
    } 
    // Si le dossier finit par '/', on le force en is_directory même si stat échoue
    // else if (!url_path.empty() && url_path[url_path.size() - 1] == '/') {
    //     is_directory = true;
    // }

    // ÉTAPE 2 : Si c'est un dossier, on cherche obligatoirement dans le Body
    if (is_directory) {
        std::string body = this->request_->getBody();
        size_t id = body.find("Content-Disposition:");
        if (id == std::string::npos) return 1;



		size_t start = body.find("filename=", id);
		if (start == std::string::npos)
			return 1;
		start += 9;
		while (body[start] == ' ')
			++start;
		bool	quote = false;
		if (body[start] == '\"')
		{
			++start;
			quote = true;
		}
		size_t end = body.find("\r\n", start);
		if (end == std::string::npos)
			return 1;
		while (quote == true)
		{
			if (body[end - 1] == '\"')
				quote = false;
			--end;
		}
		size_t	s = body.find_last_of('/', end);
		if (s != std::string::npos)
			start = s + 1;
		std::string file_name = body.substr(start, end - start);

        // On construit le chemin final : Dossier + / + Nom
        this->post_file_name_ = url_path;
        if (this->post_file_name_[this->post_file_name_.size() - 1] != '/')
            this->post_file_name_ += '/';
        this->post_file_name_ += file_name;
    } 
    // ÉTAPE 3 : Si ce n'est pas un dossier, le nom est déjà dans l'URL
    else {
        this->post_file_name_ = url_path;
    }

    // ÉTAPE 4 : Vérification finale - Est-ce que le dossier parent existe ?
    size_t last_slash = this->post_file_name_.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir_to_check = this->post_file_name_.substr(0, last_slash);
        if (stat(dir_to_check.c_str(), &s) != 0 || !S_ISDIR(s.st_mode)) {
            std::cerr << "Erreur : Le dossier de destination n'existe pas : " << dir_to_check << std::endl;
            return (1);
        }
    }

    std::cout << "Fichier final retenu : " << this->post_file_name_ << std::endl;
    return (0);
}

void	RequestAnswer::fullAnswer()
{
	std::string header = request_->getVersion() + ' ' + itoa(code_);
	if (this->code_ == 200)
		header += " OK\r\n";
	else if (this->code_ == 201)
		header += " Created\r\n";
	else if (this->code_ == 301)
		header += " Moved\r\n";
	else
	{
		header += " Not Found\r\n";
		this->content_type_ = "text/html";
	}
	header += "Content-Type: " + content_type_ + "\r\n";
	header += "Content-Length: " + itoa(body_.length()) + "\r\n";
	header += "\r\n";
	//header += "Connection: close\r\n";
	this->answer_ = header + this->body_;
}

AnswerStatus	RequestAnswer::setAnswer(Request &request)
{
	this->request_ = &request;
	this->answer_.clear();
	AnswerStatus	status = ERROR;

	if (this->request_->getMethod() == "GET")
	{

		status = methodGet();
	}

	else if (this->request_->getMethod() == "DELETE")
	{
		if (unlink(this->request_->getPath().c_str()) != 0)
		{
			std::cout << "error 404 error supression"  << std::endl;
			error_ = 404;
			code_ = 404;
			status = (ERROR);
		}
		else
		{
			status = READY_TO_SEND;
		}
	}
	else if (request_->getMethod() == "POST")
	{
		status = methodPost();
	}
	if (status == CGI_IN_PROGRESS)
	{
		return (status);

	}
	fullAnswer();
	return (status);
}

// fonction pour déterminer si c'est un cgi
// et pour stocker l'interpreter correspondant
bool	RequestAnswer::isCgi()
{
	std::map<std::string, std::string>	cgi_handlers = request_->getLocation().getCgiHandlers();

	std::string	url = request_->getUrlPath();

	size_t	last_point_position = url.find_last_of(".");
	
	if (last_point_position == std::string::npos)
		return (false);

	std::string	extension = url.substr(last_point_position);

	std::map<std::string, std::string>::iterator it = cgi_handlers.find(extension);

	if (it != cgi_handlers.end())
	{
		// DEBUG
		//std::cout << "extension : " << it->first << std::endl;
		//std::cout << "interpreter : " << it->second << std::endl;
		//
		this->cgi_interpreter_ = it->second;
		return (true);
	}
	return (false);
}

AnswerStatus	RequestAnswer::methodCGI()
{
	this->cgi_handler_ = new CGIHandler(*(this->request_), this->cgi_interpreter_);

	try {
		this->cgi_handler_->execute();
		return (CGI_IN_PROGRESS);
	}
	catch (const std::exception &e)
	{
		std::cerr << "CGI Error : " << e.what() << std::endl;
		this->error_ = 500;
		this->code_ = 500;
		return (ERROR);
	}
}

// traducteur qui transforme la sortie brute du script CGI
// en éléments exploitables
// pour construire une vraie réponse HTTP
// body, content type et code
void	RequestAnswer::buildCGIResponse()
{
	std::string	raw = this->cgi_handler_->getRawOutput();
	size_t	separator = raw.find("\r\n\r\n");
	if (separator != std::string::npos)
	{
		std::string	headers = raw.substr(0, separator);
		this->body_ = raw.substr(separator + 4);
		size_t	start = headers.find("Content-type: ");
		if (start == std::string::npos)
			start = headers.find("Content-Type: ");
		if (start != std::string::npos)
		{
			start += 14;
			size_t end = headers.find("\r\n", start);
			this->content_type_ = headers.substr(start, end - start);
		}
		else
			this->content_type_ = "text/html";
	}
	else
	{
		this->body_ = raw;
		this->content_type_ = "text/html";
	}
	this->code_ = 200;
	this->fullAnswer();
}

bool	RequestAnswer::isResponseFullySent() const
{
	return (this->answer_.empty());
}

void	RequestAnswer::eraseSentBytes(size_t bytes_sent)
{
	if (bytes_sent <= this->answer_.length())
	{
		this->answer_.erase(0, bytes_sent);
	}
	else
	{
		this->answer_.clear();
	}
}

void	RequestAnswer::clear()
{
	this->code_ = 200;
	this->error_ = 200;
	this->request_ = NULL;
	if (this->cgi_handler_ != NULL)
	{
		delete (this->cgi_handler_);
		this->cgi_handler_ = NULL;
	}
	this->answer_.clear();
	this->content_type_.clear();
	this->body_.clear();
	this->post_file_name_.clear();
	this->cgi_interpreter_.clear();
}