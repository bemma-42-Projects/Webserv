/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 16:14:03 by julien            #+#    #+#             */
/*   Updated: 2026/04/20 16:46:38 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

# include "Request.hpp"
# include "CGISubprocess.hpp"
# include <string>
# include <vector>
# include <map>
# include <sstream>
# include <iostream>
# include <cstring>
# include <cstdlib>
# include <unistd.h>

class   CGIHandler
{
    public:
        // ajouter un constructeur par defaut ?
        // un constructeur par copie ?
        // la surcharge de l'operateur = ?
        CGIHandler(Request &request, const std::string &interpreter);
        ~CGIHandler();

        // execute a été remplacée
        // pourquoi ?
        void    execute();

        // voir plus bas : plus besoin de body, content_type et code ici
        //std::string getBody() const;
        //std::string getContentType() const;
        //int         getCode() const;

        // on ajoute la fonction getPid()
        // car le serveur devra faire un waitpid sur le PID du CGI
        // pour attendre la réponse du CGI
        // et ensuite traduire cette réponse en HTTP
        int         getPid() const;

        // on ajoute aussi getReadFd
        // car le serveur devra aussi accéder au fd du CGI
        // pour le mettre dans la liste d'epoll_events
        // EPOLL_CTL_ADD
        // etc
        // pour le process des données brutes recues du client
        int         getReadFd() const;

        // il faut gérer le fait que la réponse du CGI peut arriver en chunks !
        // il faut donc un appendOutput
        // qui prendra le chunk et le concaténera au raw output cgi
        void        appendOutput(const std::string &chunk);

        // il faut aussi bien sur un getRawOutput
        // qui sera utilisé par le traducteur réponse CGI en réponse HTTP buildCGIReponse
        std::string getRawOutput() const;

    private:
        char		**getEnvp();
		void		addHeadersToEnv(std::vector<std::string>& env_vector);
        void        freeEnvp(char **envp);

        Request     &request_;
        std::string interpreter_;

        // il faut pouvoir accéder au CGISubprocess
        // pour pouvoir accéder au pid et au read fd du processus CGI
        CGISubprocess   subprocess_;

        // body_, content_type_ et code_ font maintenant partie de RequestAnswer
        // car il faut tout d'abord traduire la réponse CGI en réponse HTTP
        // et c'est RequestAnswer qui s'en occupera
        //std::string body_;
        //std::string content_type_;
        //int         code_;

        // pour cette même raison, nous avons maintenant cgi_raw_output
        // qui est la réponse CGI du cgi_handler
        // avant sa traduction
        // c'est une string
        // il faut aussi gérer le cas où la réponse arrive en chunks
        // cette variable sera alimentée au fur et à mesure par ces chunks
        std::string cgi_raw_output_;
};

#endif