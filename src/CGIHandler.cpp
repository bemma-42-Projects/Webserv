/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 16:25:09 by julien            #+#    #+#             */
/*   Updated: 2026/04/20 16:51:56 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"

CGIHandler::CGIHandler(Request &request, const std::string &interpreter) : request_(request), interpreter_(interpreter), code_(200)
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


// en CGI, le seul moyen de communication entre le serveur
// et le script PHP (avant son exécution)
// sont les variables d'environnement
char		**CGIHandler::getEnvp()
{
	std::vector<std::string>	env;
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REQUEST_METHOD=" + request_.getMethod());
	env.push_back("REQUEST_URI=" + request_.getRequestUri());
    env.push_back("SCRIPT_FILENAME=" + request_.getPath());
	env.push_back("SCRIPT_NAME=" + request_.getUrlPath());
    env.push_back("QUERY_STRING=" + request_.getQueryString());
	env.push_back("CONTENT_TYPE=" + request_.getContentType());
    std::stringstream	ss_len;
	ss_len << request_.getContentLength();
	env.push_back("CONTENT_LENGTH=" + ss_len.str());
	env.push_back("SERVER_NAME=" + request_.getHost());
	std::stringstream	ss_port;
	ss_port << request_.getPort();
	env.push_back("SERVER_PORT=" + ss_port.str());
	env.push_back("REMOTE_ADDR=" + request_.getClientIP());
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
	char	        **envp = this->getEnvp();
	std::string     raw_cgi_output;
    CGISubprocess   subprocess;

	try {    
        subprocess.createSubprocess(this->request_.getPath(), this->interpreter_, envp);
		
		if (this->request_.getMethod() == "GET")
			close(subprocess.getWriteFd());
		raw_cgi_output = subprocess.readResponse();
		this->parseCgiOutput(raw_cgi_output);
    } catch (const std::exception& e) {
        this->freeEnvp(envp);
    }
    this->freeEnvp(envp);
}

// pour séparer les headers du body
void	CGIHandler::parseCgiOutput(const std::string &raw)
{
	size_t	separator = raw.find("\r\n\r\n");

	if (separator != std::string::npos)
	{
		std::string headers = raw.substr(0, separator);
		// pour sauter le \r\n\r\n
		this->body_ = raw.substr(separator + 4);
		if (headers.find("Content-type: ") != std::string::npos) {
            size_t start = headers.find("Content-type: ") + 14;
            size_t end = headers.find("\r\n", start);
            this->content_type_ = headers.substr(start, end - start);
        }
	}
	else
		this->body_ = raw;
	this->code_ = 200;
}

std::string CGIHandler::getBody() const
{
    return (body_);
}

std::string CGIHandler::getContentType() const
{
    return (content_type_);
}

int CGIHandler::getCode() const
{
    return (code_);
}

