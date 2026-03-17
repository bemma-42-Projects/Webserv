#ifndef LOCATION_CONFIG_HPP
# define LOCATION_CONFIG_HPP

# include <string>
# include <vector>
# include <map>

class   LocationConfig {
    public:
        LocationConfig();
        ~LocationConfig();

    private:
        std::string                         _path;
        std::string                         _root;
        bool                                _autoindex;
        std::string                         _client_max_body_size;

        // On utilise vector car l'ordre est crucial :
        // exemple : index index.html index.php
        // le serveur va d'abord chercher index.html, puis, s'il ne le trouve pas, va chercher index.php
        // vector conserve l'ordre exact dans lequel les elements ont ete inseres lors du parsing
        std::vector<std::string>            _index;

        // exemple : error_page 404 /errors_pages/404.html
        // la cle est le code d'erreur, la valeur est l'URI
        // de plus, map ecrase les doublons automatiquement
        std::map<int, std::string>          _error_page;

        // exemple : cgi_handler .php /usr/bin/php-cgi
        // une map est adaptee dans ce cas
        // plus rapide pour relier 2 valeurs (strings ici) plutot que de faire des if else
        std::map<std::string, std::string>  _cgi_handler;

        // limit except est la liste des methodes autorisees (GET ou/et POST ou/et DELETE)
        // quand une requete arrive, le serveur devra parcourir cette liste pour voir si la methode demandee s'y trouve
        // un vector est adapte car c'est une structure legere et rapide faite pour ce genre de cas de figure
        std::vector<std::string>            _limit_except;

        std::string                         _upload_store;

        // return prend 1 ou 2 arguments
        // un code HTTP ou/et une URL de redirection
        // exemples :
        // return 301 https://www.google.com
        // return 403
        // return 200 "ok";
        // ici, on a une relation stricte entre 2 elements
        // pas de map, car le serveur ne fait pas de recherche dans return
        // mais execute immediatement
        // pair contient un first et un second, il est leger et parfait pour
        // coupler deux variables d'un seul coup
        // remarque : s'il n'y a qu'un parametre, on mettra le deuxieme a ""
        std::pair<int, std::string>         _return;
}

#endif