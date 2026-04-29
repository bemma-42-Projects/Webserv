/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 16:25:09 by julien            #+#    #+#             */
/*   Updated: 2026/04/28 14:32:27 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include <string>

// attention : le PID de l'interpreter cgi
// provient du CGISubproccess !
// il faut donc inclure CGISubprocess
// le read fd aussi !
#include "CGISubprocess.hpp"

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

// en CGI, le seul moyen de communication entre le serveur
// et le script PHP (avant son exécution)
// sont les variables d'environnement
char		**CGIHandler::getEnvp()
{
	std::vector<std::string>	env;
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REQUEST_METHOD=" + this->request_.getMethod());
	env.push_back("REQUEST_URI=" + this->request_.getRequestUri());

	char	cwd[1024];
	std::string	absolutePath;

	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		std::string	currentDir(cwd);
		std::string	relativePath = this->request_.getPath();

		if (relativePath.size() >= 2 && relativePath[0] == '.' && relativePath[1] == '/')
		{
			relativePath = relativePath.substr(1);
		}
		if (!relativePath.empty() && relativePath[0] == '/')
			absolutePath = currentDir + relativePath;
		else
			absolutePath = currentDir + "/" + relativePath;
	}
	else
		absolutePath = this->request_.getPath();
	env.push_back("SCRIPT_FILENAME=" + absolutePath);


	env.push_back("PATH_TRANSLATED=" + this->request_.getPath());

	env.push_back("DOCUMENT_ROOT=" + this->request_.getLocation().getRoot());

	env.push_back("SCRIPT_NAME=" + this->request_.getUrlPath());
    env.push_back("QUERY_STRING=" + this->request_.getQueryString());
	env.push_back("CONTENT_TYPE=" + this->request_.getContentType());
    
	std::stringstream	ss_len;
	ss_len << request_.getContentLength();
	env.push_back("CONTENT_LENGTH=" + ss_len.str());
	
	env.push_back("SERVER_NAME=" + this->request_.getHost());
	
	std::stringstream	ss_port;
	ss_port << this->request_.getPort();
	env.push_back("SERVER_PORT=" + ss_port.str());
	
	env.push_back("REMOTE_ADDR=" + this->request_.getClientIP());
	env.push_back("REDIRECT_STATUS=200");
	this->addHeadersToEnv(env);

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
	// alloue le tableau de variables d'environnement
    char    **envp = this->getEnvp();

        // --- LOGS DE DEBUG ---
    /*
	std::cout << "--- CGI ENVP LOGS ---" << std::endl;
    if (envp) {
        for (int i = 0; envp[i]; i++) {
            std::cout << "[ENV] " << envp[i] << std::endl;
        }
    }
    std::cout << "----------------------" << std::endl;
    // ----------------------
	*/
    try {
		// crée le fork et appelle execve
		// crée aussi les pipes pour relier la sortie du script au serveur
        this->subprocess_.createSubprocess(this->request_.getPath(), this->interpreter_, envp);

		// si c'est GET, on envoie pas le corps de la requete
		// on ferme le pipe d'ecriture
        if (this->request_.getMethod() == "GET")
                close(this->subprocess_.getWriteFd());
    }
	// liberation de la memoire en cas d'erreur
	catch (const std::exception& e) {
        this->freeEnvp(envp);
            throw;
    }
	// liberation de la memoire a la fin
    this->freeEnvp(envp);
}
