#pragma once
#include <string>
#include "Request.hpp"
#include "CGISubprocess.hpp"

// fix : include Location
#include "Location.hpp"

// NOUVEAU :
// il faut relier Answer (RequestAnswer) au CGIHandler !
// 1 : car la réponse CGI met un certain temps à arriver
// 2 : car elle provient d'un interpreter qui est un sous-processus
// 3 : car la réponse CGI doit être traduite en réponse HTTP
// 4 : car le FD de l'interpreter CGI doit être ajouté dans la liste epoll
// pour toutes ces raisons, il faut gérer le CGI autrement que n'importe quelle autre requête
// et donc aussi gérer le statut de la réponse autrement (voir AnswerStatus plus bas)

// Ajouter l'include de CGIHandler.hpp pour intégrer cgi_handler (de type CGIHandler)
#include "CGIHandler.hpp"

// Mettre en place un état CGI_IN_PROGRESS
// car si l'answer est un CGI, il faut le gérer autrement qu'un READY_TO_SEND
// remplacer le retour 0 (error) par l'état ERROR
// et le retour 1 par READY_TO_SEND

enum	AnswerStatus
{
	ERROR = 0,
	READY_TO_SEND = 1,
	CGI_IN_PROGRESS = 2
};

class RequestAnswer
{
	public:
		// Pour la forme canonique, ajouter le constructeur par défaut RequestAnswer()
		RequestAnswer();
		// Supprimer le constructeur paramétrique RequestAnswer(Request &request);
		// nous utiliserons setAnswer à la place, qui prendra Request en paramètre
		// et retournera un int selon l'AnswerStatus
		//RequestAnswer(Request &request);

		// pour la forme canonique, ajouter le constructeur par copie
		RequestAnswer(const RequestAnswer &src);

		// pour la forme canonique, ajouter aussi la surcharge de l'opérateur =
		RequestAnswer	&operator=(const RequestAnswer &rhs);

		~RequestAnswer();

		// setAnswer prend maintenant en paramètre &request
		int			setAnswer(Request &request);
		int			methodGet();
		int			methodPost();
		void		fullAnswer();
		int			getIfFile(std::string file);
		int			getIfDir();
		const std::string	&getAnswer() const;
		int			getError() const;
		// Ajouter aussi le getter CGIHandler *getCGIHandler
		CGIHandler	*getCGIHandler() const;

		std::string findIndex(Location loc);
		std::string findContentType(const std::string& path);
		int			fileName();
		bool		isCgi();
		int			methodCGI();
		
		// Ajouter buildCGIResponse
		// ca sera le "pont" de traduction entre le script et le protocole HTTP
		// car l'interpréteur CGI ne génèrera pas une réponse HTTP complète
		// il génèrera une sortie CGI
		// exemple :
		// Content-type: text/html; charset=UTF-8\r\n
		//	\r\n
		// <h1>Coucou</h1>
		// il manque la ligne de statut (exemple : HTTP/1.1 200 OK)
		// et d'autres entêtes
		// ce traducteur doit intercepter le texte brut généré par le CGI
		// lire les entêtes CGI
		// séparer ces instructions du vrai contenu (HTML par exemple)
		// et préparer le terrain pour que le serveur construise une vraie réponse HTTP
		void		buildCGIResponse();

		// voir aussi pour fix le probleme
		// de la reponse gardee en memoire
		// bug avec une demande d'une requete gardee en memoire
		// par exemple, demande de index.php
		// puis index.html
		// ensuite, index.php renvoie index.html
		// voir comment supprimer la requete ou sa reponse
		// supprimer les buffers
		// au moins la reponse
		
		// voir si la reponse est vide ?
		// cas possible ou non ? comment gerer ca ?

		// responsable de l'envoi asynchrone
		// indique au serveur si le travail est terminé ou s'il faut attendre le prochain tour d'epoll
		// 1 : premier tour d'epoll : le buffer fait 1000 octets par exemple
		// le reseau est lent, donc il n'accepte d'envoyer que 400 octets
		// on execute eraseSentBytes(400), answer ne fait donc plus que 600 octets
		// le serveur appelle isResponseFullySent, qui repond false
		// il ne faut donc pas deconnecter le client
		// 2eme tour, le send envoie tout
		// on eraseSentBytes(600)
		// le buffer est vide
		// isResponseFullySent renvoie true
		// le serveur peut repasser en mode ecoute
		// (Keep-Alive)
		bool	RequestAnswer::isResponseFullySent() const;



	private:
		int				code_;//code de sorti ou error
		int				error_;
		// ATTENTION !
		// j'ai remplacé request_
		// par *request_
		// il faut donc utiliser a partir de maintenant
		// this->request_->getPath()
		// par exemple
		// au lieu de this->request_.getPath()
		Request			*request_;
		// on ajoute le cgi_handler_ en private
		CGIHandler		*cgi_handler_;

		std::string		answer_;//ne pas oublier la ligne vide
		std::string		content_type_;//type de retour (image txt...)
		std::string		body_;		
		std::string		post_file_name_;
		std::string		cgi_interpreter_;
};
