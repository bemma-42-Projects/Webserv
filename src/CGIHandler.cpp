/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 16:25:09 by julien            #+#    #+#             */
/*   Updated: 2026/05/13 17:06:00 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include <string>
#include <climits> // Pour PATH_MAX
#include <cstdlib> // Pour realpath

#include "CGISubprocess.hpp"
#include "parsingconf.hpp"

CGIHandler::CGIHandler(Request &request, const std::string &interpreter) : request_(request), interpreter_(interpreter), cgi_raw_output_("")
{
    
}

CGIHandler::~CGIHandler()
{

}

void    CGIHandler::freeEnvp(char **envp)
{
    if (!envp)
    {
        return ;
    }
	int i = 0;
	while (envp[i] != NULL)
	{
		delete[] envp[i];
		i++;
	}
	delete[] envp;
}

// pour obtenir le Pid de l'interpreteur CGI
// utile pour que le serveur puisse waitpid la réponse CGI
int	CGIHandler::getPid() const
{
	return (this->subprocess_.getPid());
}

// pour obtenir le fd de lecture du CGI
// car le serveur devra aussi accéder au fd du CGI
// pour le mettre dans la liste d'epoll_events
int	CGIHandler::getReadFd() const
{
	return (this->subprocess_.getReadFd());
}

int	CGIHandler::getWriteFd() const
{
	return (this->subprocess_.getWriteFd());
}

// il faut gérer le fait que la réponse du CGI peut arriver en chunks !
// il faut donc un appendOutput
// qui prendra le chunk et le concaténera au raw output cgi
void	CGIHandler::appendOutput(const std::string &chunk)
{
	this->cgi_raw_output_ += chunk;
}

// il faut aussi bien sur un getRawOutput
// qui sera utilisé par le traducteur réponse CGI en réponse HTTP buildCGIReponse
std::string CGIHandler::getRawOutput() const
{
	return (this->cgi_raw_output_);
}

size_t	CGIHandler::getBytesSent() const
{
	return (this->bytes_sent_);
}

void CGIHandler::handleWrite()
{
    const std::string& body = this->request_.getBody();
    size_t total_size = body.size();
    
    ssize_t bytes = write(this->subprocess_.getWriteFd(), 
                          body.c_str() + bytes_sent_, 
                          total_size - bytes_sent_);

    if (bytes > 0)
    {
        bytes_sent_ += bytes;
    }

    if (bytes_sent_ >= total_size)
        close(this->subprocess_.getWriteFd());
}

/*
std::string	CGIHandler::getAbsolutePath_() const
{
	return (this->request_.getPath());
}
*/

std::string CGIHandler::getAbsolutePath_() const
{
    char abs_path[PATH_MAX];
    if (realpath(this->request_.getPath().c_str(), abs_path))
        return (std::string(abs_path));
    return (this->request_.getPath());
}

void	CGIHandler::setupStandardEnv_(std::vector<std::string> &env) const
{
	std::cout << "\033[1;33m[CGI DEBUG] Tentative de configuration de l'env...\033[0m" << std::endl;
	std::stringstream	ss_len;
	std::stringstream	ss_port;

	ss_len << this->request_.getBody().size();
	ss_port << this->request_.getPort();

	const LocationConfig	&loc = this->request_.getLocation();

	std::string				document_root;
	
	std::string	alias;
	if (loc.getPath() == "/directory/" || loc.getPath() == "/directory") {
        char abs_path[PATH_MAX];
        if (realpath("YoupiBanane", abs_path))
            document_root = std::string(abs_path);
        else
            document_root = "YoupiBanane"; 
    }
    else {
        document_root = loc.getRoot();
        if (document_root.empty())
            document_root = this->request_.getServer()->getRoot();
    }
	
	if (document_root.empty())
		document_root = this->request_.getServer()->getRoot();

	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REQUEST_METHOD=" + this->request_.getMethod());
	env.push_back("REQUEST_URI=" + this->request_.getRequestUri());
	
	env.push_back("SCRIPT_FILENAME=" + this->getAbsolutePath_());
	
	env.push_back("PATH_TRANSLATED=" + this->getAbsolutePath_());
	
	env.push_back("PATH_INFO=" + this->request_.getUrlPath());
	
	env.push_back("DOCUMENT_ROOT=" + document_root);
	
	env.push_back("SCRIPT_NAME=" + this->request_.getUrlPath());
    env.push_back("QUERY_STRING=" + this->request_.getQueryString());
	env.push_back("CONTENT_TYPE=" + this->request_.getContentType());
    env.push_back("CONTENT_LENGTH=" + ss_len.str());
	env.push_back("SERVER_NAME=" + this->request_.getHost());
	env.push_back("SERVER_PORT=" + ss_port.str());
	env.push_back("REMOTE_ADDR=" + this->request_.getClientIP());
	env.push_back("REDIRECT_STATUS=200");
}

char	**CGIHandler::vectorToCharArray_(const std::vector<std::string> &env) const
{
	char			**envp = new char*[env.size() + 1];
	std::size_t		i = 0;

	while (i < env.size())
	{
		envp[i] = new char[env[i].length() + 1];
		strcpy(envp[i], env[i].c_str());
		i++;
	}
	envp[env.size()] = NULL;
    return (envp);
}


// en CGI, le seul moyen de communication entre le serveur
// et le script PHP (avant son exécution)
// sont les variables d'environnement
char		**CGIHandler::getEnvp()
{
	std::vector<std::string>	env;

	this->setupStandardEnv_(env);
	std::cout << "\033[1;34m[CGI ENV LOG]\033[0m" << std::endl; // En bleu pour y voir clair
	for (std::vector<std::string>::iterator it = env.begin(); it != env.end(); ++it) {
    	std::cout << "  " << *it << std::endl;
	}
	std::cout << "\033[1;34m[END CGI ENV]\033[0m" << std::endl;
	this->addHeadersToEnv(env);

	return (this->vectorToCharArray_(env));
}

// cette fonction convertit les headers envoyés par le client
// en variables d'environnement
// pour le CGI
	// ajoute le préfixe HTTP_
	// convertit le nom en majuscules
	// remplace "-" par "_"
void    CGIHandler::addHeadersToEnv(std::vector<std::string>& env_vector)
{
	std::map<std::string, std::string>	headers = request_.getHeaders();
	std::map<std::string, std::string>::iterator	it = headers.begin();
	while (it != headers.end())
	{
		std::string	key = it->first;
		std::string	value = it->second;
		if (key == "Content-Type" || key == "Content-Length")
		{
			++it;
			continue;
		}
		std::string	env_key = "HTTP_";
		size_t	i = 0;
		while (i < key.length())
		{
			if (key[i] == '-')
				env_key += '_';
			else
				env_key += toupper(key[i]);
			i++;
		}
		env_vector.push_back(env_key + "=" + value);
		++it;
	}
}

// fonction pour exécuter le CGI
void    CGIHandler::execute()
{
	if (access(this->interpreter_.c_str(), X_OK) == -1) {
        throw std::runtime_error("CGI interpreter not found or not executable");
    }

    char    **envp = this->getEnvp();
	// --- LOG DES VARIABLES D'ENVIRONNEMENT ---
    std::cout << "\033[1;35m[CGI EXECUTE DEBUG]\033[0m" << std::endl;
    std::cout << "Interpréteur : " << this->interpreter_ << std::endl;
    std::cout << "Script Path  : " << this->getAbsolutePath_() << std::endl;
    std::cout << "\033[1;34m--- Environment Variables ---\033[0m" << std::endl;
    if (envp) {
        for (int i = 0; envp[i]; ++i) {
            std::cout << "  " << envp[i] << std::endl;
        }
    }
    std::cout << "\033[1;34m-----------------------------\033[0m" << std::endl;
    try {
		this->subprocess_.createSubprocess(this->getAbsolutePath_(), this->interpreter_, envp);
		
		this->bytes_sent_ = 0;
		
		if (this->request_.getMethod() != "POST" || this->request_.getBody().empty())
		{
			if (this->subprocess_.getWriteFd() != -1)
				close(this->subprocess_.getWriteFd());
		}
    }
	catch (const std::exception& e) {
		this->freeEnvp(envp);
            throw;
    }
    this->freeEnvp(envp);
}
