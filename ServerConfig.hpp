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

        const std::vector<std::string>              &getListen() const;
        const std::vector<std::string>              &getServerName() const;
        const std::string                           &getRoot() const;
        const std::vector<std::string>              &getIndex() const;
        const std::map<int, std::string>            &getErrorPage() const;
        const std::vector<LocationConfig>           &getLocations() const;
        bool                                        getAutoIndex() const;
        const std::string                           &getClientMaxBodySize() const;
        const std::map<std::string, std::string>    &getCgiHandler() const;

        void                                        setRoot(const std::string &root);
        void                                        setAutoIndex(bool autoindex);
        void                                        setClientMaxBodySize(const std::string &size);

        void                                        addListen(const std::string &listen);
        void                                        addServerName(const std::string &name);
        void                                        addIndex(const std::string &index);
        void                                        addErrorPage(int code, const std::string &uri);
        void                                        addLocation(const LocationConfig &location);        
        void                                        addCgiHandler(const std::string &extension, const std::string &path);

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