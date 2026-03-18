#ifndef GLOBAL_CONFIG_HPP
# define GLOBAL_CONFIG_HPP

# include <string>
# include <vector>
# include <map>
# include "ServerConfig.hpp"

class   GlobalConfig {
    public:
        GlobalConfig();
        GlobalConfig(const GlobalConfig &src);
        GlobalConfig    &operator=(const GlobalConfig &rhs);
        ~GlobalConfig();

        const std::vector<ServerConfig>     &getServers() const;
        const std::string                   &getRoot() const;
        const std::vector<std::string>      &getIndex() const;
        const std::map<int, std::string>    &getErrorPage() const;
        bool                                getAutoIndex() const;
        const std::string                   &getClientMaxBodySize() const;

        void                                setRoot(const std::string &root);
        void                                setAutoIndex(bool autoindex);
        void                                setClientMaxBodySize(const std::string &size);
        
        void                                addServer(const ServerConfig &server);
        void                                addIndex(const std::string &index);
        void                                addErrorPage(int code, const std::string &uri);

    private:
        // Multiple allowed : yes
        std::vector<ServerConfig>   _servers;
        std::string                 _root;

        // On utilise vector car l'ordre est crucial :
        // exemple : index index.html index.php
        // le serveur va d'abord chercher index.html, puis, s'il ne le trouve pas, va chercher index.php
        // vector conserve l'ordre exact dans lequel les elements ont ete inseres lors du parsing
        std::vector<std::string>    _index;

        // exemple : error_page 404 /errors_pages/404.html
        // la cle est le code d'erreur, la valeur est l'URI
        // de plus, map ecrase les doublons automatiquement
        std::map<int, std::string>  _error_page;

        bool                        _autoindex;
        std::string                 _client_max_body_size;
};

#endif