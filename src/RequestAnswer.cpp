#include "RequestAnswer.hpp"
#include "CGIHandler.hpp"
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
#include <fstream>
#include <map>
#include <cstring>	// pour strcpy

// NOTE : ajouter this-> des que possible pour la norme
// et les return entre paretheses

// mettre error_ a 0 quand code_ est egal a 200 ?
// il est init a 0 donc c'est peut-etre pas la peine
// ou alors le faire mais juste par clarte et securite
// si ca ne fais pas de probleme avec la norme
// (le nombre de lignes dans la fonction)

// attention aussi (et dans les autres fichiers)
// aux copies par référence vs copies directe
// pour les performances
// voir stress test


// ATTENTION :
// si on demande un dossier, il faut relier a l'index !
// avec la directive index
// et si on ne trouve pas le premier index, passer au suivant
// etc etc
// voir directives index
// et le comportement avec Location
// pour l'instant, ca bug ou c'est pas encore en place ?

// voir aussi quelles infos afficher si autoindex
// quand on demande un repertoire et qu'on affiche les fichiers de ce dossier
// il faut peut-etre ajouter la date de creation du fichier, sa taille
// sa date de derniere modification...
// je ne sais pas trop, a voir avec le comportement
// reel de NGINX
// (facultatif ?)
// je ne sais pas non plus s'il faut que le nom du fichier soit un lien
// a href vers celui-ci
// a verifier
// et a tester


// faire attention aussi au cas ou on demande un dossier
// et que l'index demande est un CGI ?!
// apres avoir mis en place ou corrige l'index


// ATTENTION : comme j'ai mis en place AnswerStatus pour les CGI
// il faut retourner ces etats
// au lieu de 0 ou 1 maintenant
// et verifier que les checks soit ok
// pour eviter de faux positifs ou faux negatifs
// utiliser donc if (result == ERROR) au lieu de 
// if (result == 1) maintenant !

//  ERROR = 0,
//	READY_TO_SEND = 1,
//	CGI_IN_PROGRESS = 2

// constructeur par défaut
// on initialise le code à 200 et l'error à 0
// pas de request ni de cgi handler (NULL) pour l'instant
RequestAnswer::RequestAnswer() : code_(200), error_(0), request_(NULL), cgi_handler_(NULL)
{
	// on initialise le reste à chaine vide
	this->answer_ = "";
	this->content_type_ = "";
	this->body_ = "";
	this->post_file_name_ = "";
	this->cgi_interpreter_ = "";
}

// constructeur par copie
// on ne copie pas le cgi handler pour éviter les erreurs de double free
// lié au partage involontaire des FD
// il ne faut pas faire de shallow copy ici
// surtout que ca n'a pas de sens de copier
// un processus en cours d'exécution (le CGI ici)
// dans le serveur asynchrone !
RequestAnswer::RequestAnswer(const RequestAnswer &src) : cgi_handler_(NULL)
{
	*this = src;
}

RequestAnswer	&RequestAnswer::operator=(const RequestAnswer &rhs)
{
	if (this != &rhs)
	{
		// on nettoie l'éventuel CGI de l'objet actuel
		// avant d'écraser ses données
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

		// on s'assure que le nouvel objet n'a pas de CGI actif
		this->cgi_handler_ = NULL;
	}
	return (*this);
}

// destructeur
// si cette requete a fait appel a un script CGI
// on libere la memoire utilisee pour stocker l'objet CGIHandler
// ATTENTION : cela detruit aussi l'attribut
// CGISubprocess lié
// (CGISubprocess subprocess_)
RequestAnswer::~RequestAnswer()
{
	if (this->cgi_handler_ != NULL)
	{
		delete (this->cgi_handler_);
		this->cgi_handler_ = NULL;
	}
}

//return le reponse
// fix :
// on retourne une reference constante pour eviter la copie
// et proteger le buffer
// pour les performances
// sinon, on copiera plusieurs fois la reponse en memoire
// alors qu'elle devra peut-etre etre envoyee en plusieurs fois
const std::string	&RequestAnswer::getAnswer()
{
	return (this->answer_);
}

//return l'error
int	RequestAnswer::getError()
{
	return (this->error_);
}

// attention : itoa est réservé !
// pourquoi pas le mettre en static ?
// ou le renommer ?
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

// utiliser plutot 4096 pour la taille du buffer ?
// 

//recupere le contenue du fichier pour la methode get
//int	RequestAnswer::getMethode()
int	RequestAnswer::getIfFile(std::string file)
{
	//std::cout << Config::getRoot() + file << std::endl;
	//int	fd = open((request_->getLocation().getRoot() + '/' + file).c_str(), O_RDONLY);
	//std::cout << "dir" << std::endl;
	int	fd = open((file).c_str(), O_RDONLY);
	// voir quel code d'erreur retourner si open ne fonctionne pas ?
	// mauvais droits d'accès ou fichier n'existe pas ?
	// faire ces 2 tests ?
	if (fd == -1)
	{
		//this->error_ = 403;
		//this->code_ = 403;
		// TODO : vérifier quand est appelé getIfFile et
		// checker ERROR, plus 1
		return (ERROR);
		//return 1;
	}
	std::string	res;
	// changer pour un buffer 4096 ?
	char buffer[2000];//taille de la reponse ([4096])
	ssize_t	bytes_read;
	// ATTENTION : A RENDRE NON BLOQUANT !
	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
	{
		res.append(buffer, bytes_read);
	}
	close(fd);
	//std::cout << res << std::endl;
	this->body_ = res;
	this->code_ = 200;
	this->content_type_ = findContentType(file);
	// TODO : vérifier quand est appelé getIfFile et
	// checker READY_TO_SEND, plus 0
	return (READY_TO_SEND);
	//return 0;
}

//recupere le contenue du dossier pour la methode get

// ATTENTION : faire la distinction avec displayName ?
// et voir plus haut pour les remarques a propos des infos supplementaires
// a afficher
// eventuellement
int	RequestAnswer::getIfDir()
{
	// std::cout << "pd" << std::endl;
	DIR* dir = opendir(request_->getPath().c_str());
	if (!dir)
	{
		// vérifier que le code soit bien 403 ou 404 ?
		// ou alors vérifier avec une autre méthode supplémentaire
		// pour retourner le bon code d'erreur
		// 404 : fichier inexistant ?
		// 403 : pas les droits ?
		// A verifier
		std::cout << "error 403" << std::endl;
		this->error_ = 403;
		this->cgi_handler_ = 403;
		// TODO : vérifier quand est appelé getIfDir et
		// checker ERROR, plus 1
		return (ERROR);
		//return 1;// Erreur 403 ou 404
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
		// code 400 ?
		// ATTENTION : il faudrait peut-etre
		// faire un continue
		// plutot que de retourner directement une erreur
		// car si un des fichiers n'existe pas
		// il ne faut pas "faire planter" tout le directory
		// et lister quand meme les autres fichiers ?
		// voir selon les bonnes pratiques
		// et selon le comportement de NGINX
		else
		{
			std::cout << "error 400" << std::endl;
			this->error_ = 400;
			this->code_ = 400;
			// TODO : vérifier quand est appelé getIfDir et
			// checker ERROR, plus 1
			return (ERROR);
			//return 1;
		} 
		// Le lien href doit être le nom, mais le texte affiché est displayName
		// donc remplacer un des name par displayName ?
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
int	RequestAnswer::methodGet()
{
	struct stat info;
	if (stat(request_->getPath().c_str(), &info) != 0)
	{
		// idem, verifier 404 ou 403 ?
		std::cerr << "error 404" << std::endl;
		this->error_ = 404;
		this->code_ = 404;
		// TODO : vérifier quand est appelé methodGet et
		// checker ERROR, plus 1
		return (ERROR);
		//return 1;
	}
	//std::string res;
	// si c'est un REGULAR FILE
	if (S_ISREG(info.st_mode))
	{
		// si c'est un CGI
		if (this->isCgi())
			// on exécute la méthode CGI
			// ATTENTION : methodCGI de RequestAnswer appelle execute
			// de CGIHandler
			// mais je verifie dans execute si c'est un GET
			// et je ne sais pas encore si c'est necessaire ou inutile
			// et a quel niveau faire cette verif
			// pour savoir si je dois faire plutot un CGI GET ou un CGI POST
			// le CGI POST n'est pas encore en place, verifier
			// avec Romane quand il sera mis en place et valide
			// pour y brancher le CGI
			// Note pour plus tard :
			// brancher aussi DELETE CGI ?

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
		// std::cout << "dir" << std::endl;
		if (!index.empty())
		{
			return (getIfFile(Config::getRoot() + '/' + index));	
			// Sinon, renvoie la page par défaut (ex: index.html).
		}
		else if (loc.getAutoindex() == true)
			return (getIfDir());
		// pourquoi 403 ici ?
		else
		{
			this->error_ = 403;
			this->code_ = 403;
		}
	}
	// TODO : vérifier quand est appelé methodGet et
	// checker ERROR, plus 1
	return (ERROR);
	//return 1;
}

// pas encore checke
// en attente de validation par Romane pour
// integrer les CGI POST (ici ou autre part, de facon puks generale ?)

// attention :
// verifier que j' (Julien) ai bien mis les bons return
// ERROR ou READY_TO_SEND
// avec Romane
// (de toutes facons ce code va peut-etre changer ou
// je dois le comprendre
// pour implementer CGI POST)
int	RequestAnswer::methodPost()
{
	Location	loc = request_->getLocation();
	std::string root = loc.getRoot() + loc.getPath();
	struct stat s;
	if (stat(root.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		std::cout << "deb" << std::endl;
		struct stat p;
		stat(request_->getPath().c_str(), &p);
		if (p.st_mode & S_IFREG)
		{
			post_file_name_ =  request_->getPath();
			std::cout << post_file_name_ << std::endl;
			// idem
			return (ERROR);
			//return 0;
		}
		bool	quote = false;
		std::string body = request_->getBody();
		size_t	id = body.find("Content-Disposition:");
		if (id == std::string::npos)
			return 1;
		size_t start = body.find("filename=", id);
		if (start == std::string::npos)
		{
			// idem
			return (READY_TO_SEND);
			//return 1;
		}
		start += 9;
		while (body[start] == ' ')
			++start;
		if (body[start] == '\"')
		{
			++start;
			quote = true;
		}
		size_t end = body.find("\r\n", start);
		if (end == std::string::npos)
		{
			// idem
			return (READY_TO_SEND);
			//return 1;
		}
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
		std::cout << "file name = " << file_name << std::endl;
		// struct stat f;
		std::string test = root + '/' + file_name;
		std::cout << "test = " << test << std::endl;

		struct stat b;
		stat(test.c_str(), &b);
		if (b.st_mode & S_IFREG)
		{
			post_file_name_ =  test;
			std::cout << " file name = " << post_file_name_ << std::endl;
			// idem
			return (ERROR)
			//return 0;
		}
	}
	std::cout << "error" << std::endl;
	// idem
	return (READY_TO_SEND);
	//return 1;
}

// Julien :
// je n'ai pas checke cette methode
// pas besoin je pense pour CGI
// mais a garder en tete si bug
// dur a trouver

//recupere le path du file name pour upload les fichier
int RequestAnswer::fileName()
{
    Location    loc = request_->getLocation();
    std::string root_path = loc.getRoot() + loc.getPath(); // Chemin dossier sur disque
    std::string url_path = request_->getPath();           // Chemin demandé dans l'URL

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
        std::string body = request_->getBody();
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
        post_file_name_ = url_path;
        if (post_file_name_[post_file_name_.size() - 1] != '/')
            post_file_name_ += '/';
        post_file_name_ += file_name;
    } 
    // ÉTAPE 3 : Si ce n'est pas un dossier, le nom est déjà dans l'URL
    else {
        post_file_name_ = url_path;
    }

    // ÉTAPE 4 : Vérification finale - Est-ce que le dossier parent existe ?
    size_t last_slash = post_file_name_.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir_to_check = post_file_name_.substr(0, last_slash);
        if (stat(dir_to_check.c_str(), &s) != 0 || !S_ISDIR(s.st_mode)) {
            std::cerr << "Erreur : Le dossier de destination n'existe pas : " << dir_to_check << std::endl;
            return 1;
        }
    }

    std::cout << "Fichier final retenu : " << post_file_name_ << std::endl;
    return 0;
}


// quelle fonction POST garder ?
// celle-ci ou l'autre plus haut ?

/*
int RequestAnswer::methodPost()
{
    if (fileName() == 1) 
    {
        error_ = 400;
        code_ = 400;
        return 1;
    }

    // DEBUG : Affiche le chemin exact que le serveur essaie d'ouvrir
    std::cout << "Tentative d'ouverture de : [" << post_file_name_ << "]" << std::endl;

    std::ofstream outfile(post_file_name_.c_str(), std::ios::out | std::ios::binary);

    if (!outfile.is_open()) {
        std::cerr << "ERREUR : Impossible d'ouvrir le fichier. Verifiez que le dossier existe et les permissions." << std::endl;
        error_ = 500;
		code_ = 500;
        return 1;
    }

    const std::string& body = request_->getBody();
    size_t startPos = body.find("\r\n\r\n");

    // Correction de la condition : on veut entrer ici si on A TROUVÉ \r\n\r\n
    if (startPos != std::string::npos) {
        startPos += 4; // On saute les deux \r\n\r\n
        
        size_t endPos = body.find("\r\n--", startPos); 
        size_t fileSize;

        if (endPos == std::string::npos) {
            fileSize = body.size() - startPos;
        } else {
            fileSize = endPos - startPos;
        }

        outfile.write(&body[startPos], fileSize);
    } 
    else {
        // Cas où ce n'est pas du multipart (données brutes)
        outfile.write(body.c_str(), body.size());
    }
    
    outfile.close();
    code_ = 201; 
    return 0;
}
*/

// voir les autres codes d'erreur ?
// modifier itoa par autre chose
// car itoa est reserve
void	RequestAnswer::fullAnswer()
{
	std::string header = request_->getVersion() + ' ' + itoa(code_);
	if (this->code_ == 200)
		header += " OK\r\n";
	else if (this->code_ == 201)
		header += " Created\r\n";
	else if (this->code_ == 301)
		header += " Moved\r\n";
	// etc etc ...
	else
	{
		header += " Not Found\r\n";
		this->content_type_ = "text/html";
	}
	header += "Content-Type: " + content_type_ + "\r\n";
	header += "Content-Length: " + itoa(body_.length()) + "\r\n";
	
	// ATTENTION : je ne sais pas si c'est \r\n\r\n
	// ou \r\n
	// qu'il faut utiliser ici
	// pour une ligne vide entre header et body
	// a verifier
	//header += "\r\n";
	header += "\r\n\r\n";

	//std::cout << "header = " << header << std::endl;

	this->answer_ = header + this->body_;
}

//envoie les fonction par rapport au methode (get, post, delete)
// on ne peut plus donner la requete au constructeur de RequestAnswer !
// il faut la donner ici !
// a la fonction qui lance le traitement
// car on utilise maintenant le late binding
// voir ci-dessous
int	RequestAnswer::setAnswer(Request &request)
{
	// ATTENTION :
	// pour respecter la problematique d'epoll
	// et de l'asynchrone, avec le cycle de vie des objets
	// il faut ajouter la ligne suivante :
	// this->request_ = &request;
	// car il faut prendre en compte que la réponse HTTP
	// n'arrive presque jamais en une seule fois !
	// il faut la concaténer
	// car elle arrive en chunks
	// et petits bouts par petits bouts
	// en attendant, d'autres clients connectés en même temps
	// peuvent recevoir d'autres petits bouts de réponse
	// c'est bien géré par epoll au niveau du serveur
	// et par le fait que les sockets soient non bloquants
	// mais le probleme qui en resulte
	// est que on ne peut pas lier la Request a RequestAnswer au moment
	// de la creation du client
	// il faut utiliser un "late binding"
	// epoll remplit Request en plusieurs tours de boucle
	// puis on branche la requete complete avec cette ligne
	// comme ca, on est sur que la requete est complete
	// avant de la brancher !
	this->request_ = &request;
	this->answer_.clear();

	// status sert a "capturer" le statut
	// de la requete, qu'elle soit GET, DELETE ou POST
	// on la met par defaut a ERROR
	// car on retournera le statut a la fin
	// ou ERROR sinon
	// cela sera ERROR
	// si la methode n'est ni GET, ni DELETE, ni POST
	// ATTENTION : ou est-ce qu'on verifie si la methode est
	// autorisee ?
	int	status = ERROR;

	if (this->request_->getMethod() == "GET")
	{
		// status recuperera ici ERROR (erreur de GET normal ou de GET CGI), READY_TO_SEND (succes GET) ou CGI_IN_PROGRESS (succes CGI) !
		// voir plus base, car methodGet checke isCgi
		// qui lance methodCGI
		// si c'est un CGI
		// et si ca fonctionne, le statut CGI_IN_PROGRESS sera remonté !
		// ou alors erreur
		// provenant de methodGet (GET normal)
		// ou de methodCGI (GET CGI)
		status = methodGet();
		// inutile maintenant, car le statut est
		// retourné plus bas
		//if (methodGet() != 0)
			//return 0;//error
		//else
		//	return 1;//get
	}

	else if (this->request_->getMethod() == "DELETE")
	{
		// voir avec DELETE CGI ici ?
		// ou plus haut ?
		// quelles differences avec DELETE normal ?
		if (unlink(this->request_->getPath().c_str()) != 0)
		{
			// 404 est bien le code d'erreur de DELETE ?
			// car 404 est non trouve normalement
			// et il peut y avoir peut-etre un code specifique
			// ou d'autres conditions pour lesquelles la suppression n'a pas fonctionne ?
			// par exemple unauthorized ou quelque chose comme ca ?
			std::cout << "error 404 error supression"  << std::endl;
			error_ = 404;
			code_ = 404;
			// idem, on remplace par le statut
			// au lieu de 0, donc verifier ERROR
			// READY_TO_SEND
			// ou CGI_IN_PROGRESS
			// plus bas
			status = (ERROR);
			// aussi, on ne retourne plus, mais on set le statut
			
			//return (0);//error
		}
		else
		{
			// quel code indiquer pour une suppression reussie ?
			// 200 ? ou autre ?
			// READY_TO_SEND en statut temporaire
			// mais a verifier ...
			// voir aussi quelle reponse envoyer ?
			// tester avec le serveur
			// sinon, changer le comportement et ce statut
			status = READY_TO_SEND;
		}
		//	return (2);//delete
		//Utilise unlink() pour supprimer le fichier
	}
	else if (request_->getMethod() == "POST")
	{
		status = methodPost();

	//	std::string	url = request_->getUrlPath();
	//	if (/*(url.size() >= 4 && url.substr(url.size() - 4) == ".php")
	//		|| (url.size() >= 3 && (url.substr(url.size() - 3) == ".py")
	//		|| url.substr(url.size() - 3) == ".pl")*/)
	//	{

	//		//CGI : gere dans le methodGet
			// mais peut-etre le gerer ici ?
			// a reflechir et voir avec l'inplementation de POST
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
	// ATTENTION : si ce n'est aucune de ces methodes, que faire ?
	// est-ce que c'est gere plus haut au niveau du parsing ?


	// si on a capturé un CGI IN PROGRESS
	// provenant de methodGet (GET CGI)
	// ou (plus tard, a implementer) de methodPost (POST CGI)
	if (status == CGI_IN_PROGRESS)
	{
		// on retourne ici, on ne construit pas la réponse tout de suite !
		// quand on arrive ici, on vient tout juste de faire le fork et le execve
		// pour lancer le script php (par exemple)
		// il ne faut pas appeler fullAnswer directement
		// car le body_ serait vide, et le serveur enverrai une page blanche
		// et le serveur serait bloqué le temps que le script
		// s'exécute
		// un autre client ne pourrait pas charger index.html (par exemple)
		// tant que le serveur est bloqué par l'exécution du script !
		// on mets donc en pause la requête
		// epoll surveillera le pipe de lecture (le FD renvoyé par getReadFd())
		// puis epoll_wait réveillera le serveur avec l'event EPOLLIN sur ce FD
		// et à ce moment là buildCGIResponse() sera appelé
		// il lira le pipe, re;plira le body_, changera l'état en READY_TO_SEND
		// et dira a epoll d'envoyer la page au client
		return (status);

	}
////mettre le reponse dans une answer_
	fullAnswer();
	//std::cout << body_ << std::endl;
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
		std::cout << "extension : " << it->first << std::endl;
		std::cout << "interpreter : " << it->second << std::endl;
		//
		this->cgi_interpreter_ = it->second;
		return (true);
	}
	return (false);
}

int	RequestAnswer::methodCGI()
{
	// allocation dynamique
	// pour que CGIHandler survive à la fin de la fonction
	// et tourne en arrière-plan pendant que le serveur écoute les autres requêtes etc
	this->cgi_handler_ = new CGIHandler(*(this->request_), this->cgi_interpreter_);

	try {
		this->cgi_handler_->execute();
		// avant, cgi execute etait bloquant
		// maintenant, il fait juste un fork et un execve
		// puis rend la main
		// il faut attendre que la reponse soit construite
		// par le cgi avant de set body, content_type et code !
		//this->body_ = cgi.getBody();
		//this->content_type_ = cgi.getContentType();
		//this->code_ = cgi.getCode();

		// on ne peut pas construire la réponse tout de suite
		// il faut donc prevenir epoll_wait que le CGI
		// est en cours d'exécution
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
	// on va chercher le texte qui a été lu depuis le pipe
	// relié à la sortie standard (STDOUT) du processus CGI
	// et concaténé
	std::string	raw = this->cgi_handler_->getRawOutput();

	// dans le protocole CGI, les headers sont séparés du body
	// par une ligne vide
	// on cherche l'index de cette ligne vide
	size_t	separator = raw.find("\r\n\r\n");

	// si la séquence a été trouvée
	if (separator != std::string::npos)
	{
		// on extrait les headers
		std::string	headers = raw.substr(0, separator);

		// on extrait le body (après les 4 caractères \r\n\r\n à partir de ce séparateur)
		this->body_ = raw.substr(separator + 4);

		// on cherche dans headers l'entête Content-type ou Content-Type
		// pour qu'on sache si le CGI a renvoyé du texte, une image, du html, etc
		size_t	start = headers.find("Content-type: ");
		if (start == std::string::npos)
			start = headers.find("Content-Type: ");
		
		// on extrait la valeur du Content-Type
		if (start != std::string::npos)
		{
			start += 14;
			size_t end = headers.find("\r\n", start);
			this->content_type_ = headers.substr(start, end - start);
		}
		// par sécurité, par défaut, le content-type sera text/html
		else
			this->content_type_ = "text/html";
	}
	// si on a pas trouvé de \r\n\r\n
	// on considère qu'il n'y a pas de header
	// et que tout est body
	else
	{
		this->body_ = raw;
		this->content_type_ = "text/html";
	}
	// OK code 200
	// on appelle enfin fullAnswer(), qui va construire la string finale avec la requete HTTP valide
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