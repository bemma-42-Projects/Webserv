#include "RequestAnswer.hpp"
#include <iostream>
#include <fcntl.h>    // pour open
#include <unistd.h>   // pour read, close
#include <sys/stat.h> // pour stat
#include "Config.hpp"
#include <dirent.h>
#include <sstream>
#include <fstream>

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

//trouve le content_type_
std::string RequestAnswer::findContentType(const std::string& path) 
{
    static std::map<std::string, std::string> mimeTypes;

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

}



//recupere le contenue du fichier pour la methode get
//int	RequestAnswer::getMethode()
int	RequestAnswer::getIfFile(std::string file)
{
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
	body_ = res;
	code_ = 200;
	content_type_ = findContentType(file);
	return 0; 
}

//recupere le contenue du dossier pour la methode get
int	RequestAnswer::getIfDir()
{
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
		std::string fullPath = request_.getPath() + "/" + name;
		struct stat st;
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
		body += "<li><a href=\"" + name + "\">" + name + "</a></li>\n";
	}
	body += "</ul><hr></body></html>";
	closedir(dir);
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
		std::string fullPath = root + '/' + *it;
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
	if (S_ISREG(info.st_mode))
		return (getIfFile(request_.getPath()));

	else if (S_ISDIR(info.st_mode))
	{
		Location	loc = request_.getLocation();
		std::string index = findIndex(loc);
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

//recupere le path du file name pour upload les fichier
int RequestAnswer::fileName()
{
    Location    loc = request_.getLocation();
    std::string root_path = loc.getRoot() + loc.getPath(); // Chemin dossier sur disque
    std::string url_path = request_.getPath();           // Chemin demandé dans l'URL

    struct stat s;
    bool is_directory = false;
    if (stat(url_path.c_str(), &s) == 0) {
        if (S_ISDIR(s.st_mode)) {
            is_directory = true;
        }
    }
    if (is_directory) {
        std::string body = request_.getBody();
        size_t id = body.find("Content-Disposition:");
        if (id == std::string::npos)
			return 1;
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
        post_file_name_ = url_path;
        if (post_file_name_[post_file_name_.size() - 1] != '/')
            post_file_name_ += '/';
        post_file_name_ += file_name;
    }
    else
        post_file_name_ = url_path;
    size_t last_slash = post_file_name_.find_last_of('/');
    if (last_slash != std::string::npos) 
	{
        std::string dir_to_check = post_file_name_.substr(0, last_slash);
        if (stat(dir_to_check.c_str(), &s) != 0 || !S_ISDIR(s.st_mode)) 
			return 1;
    }
    return 0;
}

int RequestAnswer::methodPost()
{
    if (fileName() == 1) 
    {
        error_ = 400;
        code_ = 400;
        return 1;
    }
    std::cout << "Tentative d'ouverture de : [" << post_file_name_ << "]" << std::endl;
    std::ofstream outfile(post_file_name_.c_str(), std::ios::out | std::ios::binary);
    if (!outfile.is_open())
	{
        std::cerr << "ERREUR : Impossible d'ouvrir le fichier. Verifiez que le dossier existe et les permissions." << std::endl;
        error_ = 500;
		code_ = 500;
        return 1;
    }
    const std::string& body = request_.getBody();
    size_t startPos = body.find("\r\n\r\n");
    if (startPos != std::string::npos) 
	{
        startPos += 4; 
        size_t endPos = body.find("\r\n--", startPos); 
        size_t fileSize;
        if (endPos == std::string::npos)
            fileSize = body.size() - startPos;
        else 
            fileSize = endPos - startPos;
        outfile.write(&body[startPos], fileSize);
    } 
    else
        outfile.write(body.c_str(), body.size());
    outfile.close();
    code_ = 201; 
    return 0;
}

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
	header += "\r\n";
	answer_ = header + body_;
}

//envoie les fonction par rapport au methode (get, post, delete)
int	RequestAnswer::setAnswer()
{
	answer_.clear();
	if (request_.getMethod() == "GET")
	{
		methodGet();
	}

	else if (request_.getMethod() == "DELETE")
	{
		if (unlink(request_.getPath().c_str()) != 0)
		{
			std::cout << "error 404 error supression"  << std::endl;
			error_ = 404;
			code_ = 404;
		}
	}
	else if (request_.getMethod() == "POST")
	{
		methodPost();

	}
	fullAnswer();
	return 1;
}

//le fichier n'existe pas, je dois verifier le dossier
//full ia, a comprendre
//mais ne marche pas a chaque fois , differentier le dossier du fichier