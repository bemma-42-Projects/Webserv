#pragma once
#include <string>
#include <map>

class Request {

	public:
		/**
         * @brief Constructeur par défaut de la requête HTTP.
         * @details Initialise un objet Request vide. Bien qu'une requête nécessite en pratique 
         * des données brutes pour être utile, ce constructeur est techniquement indispensable :
         * 1. Pour respecter la Forme Canonique Orthodoxe (norme Coplien).
         * 2. Pour permettre l'instanciation de cette classe au sein des conteneurs de la STL 
         * (comme std::map), qui requièrent la création d'un objet par défaut avant son assignation.
         */
		Request();

		
		Request(char *buffer);
		~Request();
		std::string 						getRequest() const;
		std::string 						getMethod() const;
		std::string 						getPath() const;
		std::string 						getUrlPath() const;
		std::string 						getVersion() const;
		std::map<std::string, std::string>	getHeaders() const;
		std::string 						getBody() const;
		int									getError() const;
		//int									requestHttp();							
		int									parsingHttp();
		bool								complete();
		int									initFistLine();
		int									initHeader();
		int									initBody();
		//std::string							answer();
		//std::string							methodGet();

	private:
		std::string							request_;
		std::string							method_;
		std::string							path_;
		std::string							url_path_;
		std::string							version_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;
		int									error_;
};

std::ostream& operator<<(std::ostream& out, const Request& request);