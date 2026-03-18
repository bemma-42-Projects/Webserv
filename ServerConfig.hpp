#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include <string>
# include <vector>
# include <map>
# include "LocationConfig.hpp"

class   ServerConfig {
    public:
        ServerConfig();
        ServerConfig(const ServerConfig &src);
        ServerConfig    &operator=(const ServerConfig &rhs);
        ~ServerConfig();

        const std::vector<std::string>      &getListen() const;
        const std::vector<std::string>      &getServerName() const;
        const std::string                   &getRoot() const;
        const std::vector<std::string>      &getIndex() const;

    private:
        // Multiple allowed : yes
        std::vector<std::string>            _listen;

        // Multiple allowed : yes
        std::vector<std::string>            _server_name;
        std::string                         _root;

        // On utilise vector car l'ordre est crucial :
        // exemple : index index.html index.php
        // le serveur va d'abord chercher index.html, puis, s'il ne le trouve pas, va chercher index.php
        // vector conserve l'ordre exact dans lequel les elements ont ete inseres lors du parsing
        std::vector<std::string>            _index;

        // exemple : error_page 404 /errors_pages/404.html
        // la cle est le code d'erreur, la valeur est l'URI
        // de plus, map ecrase les doublons automatiquement
        std::map<int, std::string>          _error_page;
        
        // Multiple allowed : yes
        std::vector<LocationConfig>         _locations;
        bool                                _autoindex;
        std::string                         _client_max_body_size;

        // exemple : cgi_handler .php /usr/bin/php-cgi
        // une map est adaptee dans ce cas
        // plus rapide pour relier 2 valeurs (strings ici) plutot que de faire des if else
        std::map<std::string, std::string>  _cgi_handler;
};

#endif