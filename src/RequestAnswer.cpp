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
int	RequestAnswer::getIfFile(std::string file)
{
	//std::cout << Config::getRoot() + file << std::endl;
	int	fd = open((Config::getRoot() + '/' + file).c_str(), O_RDONLY);
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
	//std::cout << "pd" << std::endl;
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
        std::string fullPath = loc.getRoot() + "/" + *it;
        
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
		return (getIfFile(request_.getUrlPath()));

	else if (S_ISDIR(info.st_mode))
		{
			Location	loc = request_.getLocation();
			//divier la fontion
			//!!!Le chemin relatif à la racine de ton serveur (l'URL). Si ton dossier webserv est la racine, l'utilisateur devrait juste voir Index of /.
			std::string index = findIndex(loc);
			//if
			if (!index.empty())
			{
				//return (index);
				
				return (getIfFile(request_.getPath() + '/' + index));	
				//Sinon, renvoie la page par défaut (ex: index.html).
			}
			else if (loc.getAutoindex() == true)
				return (getIfDir());
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




	//	//Si l'extension correspond à un script (ex: .php), tu dois préparer l'environnement (setenv) et fork() pour exécuter le CGI.
	//	/cgi-bin/script.py.py✅ Oui
	//    /cgi-bin/form.php.php✅ Oui   (.php, .py, .pl)

	//Lire Content-Type dans headers_ pour savoir ce que tu reçois
	//Vérifier l'extension de path_ pour détecter un CGI




	/*if (request_.getMethod() == "POST") {
    
    // 1. D'ABORD : Est-ce un CGI ? 
    // (On regarde l'extension du fichier demandé dans l'URL)
	//!!!
	!!
	Comment on le reconnaît ? Grâce à l'extension dans l'URL (.php, .py, .pl). => dans le fichier de config
    
	if (isCgiExtension(url)) {
        // ACTION : Lancer le CGI (fork/exec)
        // Le body est envoyé au script via son STDIN
        executeCGI();
    }
    
    // 2. ENSUITE : Est-ce un Upload ?
    // (On regarde si le Content-Type est multipart)
    else if (request_.getHeader("Content-Type").find("multipart/form-data") != std::string::npos) {
        // ACTION : Extraire le fichier du body et l'enregistrer sur le serveur
        // (Généralement dans le dossier défini par 'upload_store' dans ta config)
        handleUpload();
    }
    
    // 3. SINON : POST classique (ex: un formulaire simple ou du texte)
    else {
        // Traitement par défaut
    }
}
	


Étape A : Est-ce que l'URL finit par une extension CGI ?

OUI : Alors c'est un CGI. Peu importe s'il y a un fichier ou du texte dans le Body, tu donnes tout au script (.php ou .py) et c'est lui qui gère. Le CGI gagne toujours.

NON : On passe à l'étape B.

Étape B : Est-ce qu'on est en POST et que le Content-Type est "multipart/form-data" ?

OUI : C'est un Upload de fichier classique. Ton code C++ doit lire le Body, enlever les séparateurs (boundaries) et créer un fichier sur le disque.

NON : On passe à l'étape C.

Étape C : Le reste

C'est soit un GET classique (tu envoies une page HTML), soit un POST de formulaire simple (du texte), soit une erreur.
*/

/*Gemini a dit

C’est une excellente question qui touche au cœur de ton architecture. Le serveur ne le "sait" pas par magie : il le sait parce que tu as parsé ton fichier de configuration au démarrage.

Voici comment cette information circule dans ton programme :
1. La Phase de Parsing (Au lancement du serveur)

Quand tu lances ./webserv config.conf, ton programme lit le fichier texte. S'il voit upload_store ./www/uploads, il doit stocker cette chaîne de caractères dans un objet.

Typiquement, tu as une structure qui ressemble à ça :
C++

struct Location {
    std::string _path;          // ex: "/uploads"
    std::string _root;          // ex: "./www"
    std::string _upload_store;  // ex: "./www/storage" <--- C'est ICI !
    bool _allow_post;           // ex: true
    // ...
};

Si dans ton fichier .conf, la ligne upload_store n'existe pas pour cette location, alors ta variable _upload_store sera vide.
2. La Phase de Décision (Pendant la requête)

Quand une requête POST /uploads/image.jpg arrive, ton serveur fait une recherche :

    Trouver la bonne Location : Il cherche dans sa liste de locations celle qui correspond au début de l'URL (/uploads).

    Vérifier la directive :

        Il regarde : if (!currentLocation.getUploadStore().empty())

        Si c'est vide : Ce n'est pas un upload géré par le serveur. (C'est peut-être un CGI ou une erreur 405).

        Si ce n'est PAS vide : Le serveur se dit : "OK, j'ai l'ordre d'enregistrer ce fichier dans ce dossier précis".

3. Exemple concret dans ton code

Voici à quoi ressemble le test logique dans ta fonction qui gère le POST :
C++

// 1. On récupère la config de l'endroit demandé
Location& loc = config.getLocationForPath(request.getPath());

// 2. On vérifie si l'upload est configuré
if (request.getMethod() == "POST" && !loc.getUploadStore().empty()) {
    
    // C'EST UN UPLOAD !
    std::string fileName = extractFileName(request); 
    std::string savePath = loc.getUploadStore() + "/" + fileName;
    
    saveToFile(savePath, request.getBody());
    return Response(201); // Created
}

En résumé :

    Dans le fichier .conf : Tu écris le chemin où stocker les fichiers.

    Dans ton code : Tu vérifies si ce chemin a été rempli lors du parsing.

    Si le chemin existe : C'est un upload.

    Si le chemin n'existe pas : Tu traites la requête comme un POST normal (souvent envoyé vers un CGI).

Est-ce que dans ton code actuel, tu as déjà une classe ou une structure pour stocker les paramètres de tes "locations" (comme le root, les méthodes autorisées, etc.) ?
*/