/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGISubprocess.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 10:14:43 by julien            #+#    #+#             */
/*   Updated: 2026/04/20 11:49:34 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGISubprocess.hpp"
#include "utils.hpp"

#include <unistd.h>    // Pour : pipe(), fork(), dup2(), close(), execve()
//#include <fcntl.h>     // Pour : fcntl()
//#include <sys/wait.h>  // Pour : waitpid()
#include <cerrno>      // Pour : errno
#include <cstring>     // Pour : strerror()
#include <cstdlib>     // Pour : exit()
#include <stdexcept>   // Pour : std::runtime_error
#include <string>
#include <iostream>

// pipe_to_cgi_ : pour que Webserv écrive dedans (dans pipe_to_cgi_[1])
// le script PHP le lira (depuis pipe_to_cgi_[0]) sur son entrée standard

// pipe_from_cgi_ : pour que le script PHP écrive sa réponse HTML dedans (via STDOUT) (dans pipe_from_cgi_[1]))
// Webserv le lira (depuis pipe_from_cgi_[0]) pour l'envoyer au navigateur

// On passe les pipes en mode non bloquant
CGISubprocess::CGISubprocess()
{
    pipe_to_cgi_[0] = -1;
    pipe_to_cgi_[1] = -1;
    pipe_from_cgi_[0] = -1;
    pipe_from_cgi_[1] = -1;

    if (pipe(pipe_to_cgi_) != 0)
        throw (std::runtime_error("Failed to create pipe to CGI" + std::string(strerror(errno))));
    setNonBlocking(pipe_to_cgi_[0]);
    setNonBlocking(pipe_to_cgi_[1]);
    if (pipe(pipe_from_cgi_) != 0)
    {
        close(pipe_to_cgi_[0]);
        close(pipe_to_cgi_[1]);
        throw (std::runtime_error("Failed to create pipe from CGI" + std::string(strerror(errno))));
    }
    setNonBlocking(pipe_from_cgi_[0]);
    setNonBlocking(pipe_from_cgi_[1]);
}

void    CGISubprocess::createSubprocess(const std::string &filePathAbs, const std::string &interpreter, char **envp)
{
    // fork (créer une copie du processus courant)
    this->pid_ = fork();
    if (this->pid_ == -1)
        throw (std::runtime_error("Failed to create fork for CGI: " + std::string(strerror(errno))));

    // dans le child
    else if (this->pid_ == 0)
    {
        // change le current working directory
        // pour le script directory
        std::string parent_dir = ".";
        size_t      last_slash = filePathAbs.find_last_of('/');
        
        if (last_slash != std::string::npos)
            parent_dir = filePathAbs.substr(0, last_slash);
        
        if (chdir(parent_dir.c_str()) == -1)
        {
            std::cerr << "CGI Error: chdir failed : " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        // close l'extrémité en écriture du pipe (c'est le parent qui écrira dedans)
        close(pipe_to_cgi_[1]);
        // redirige stdin pour lire pipe_to_cgi
        if (dup2(pipe_to_cgi_[0], STDIN_FILENO) == -1)
            exit(EXIT_FAILURE);
        // close le fd (car il a été redirigé)
        close(pipe_to_cgi_[0]);
        
        // close l'extrémité en lecture du pipe (c'est le parent qui lira dedans)
        close(pipe_from_cgi_[0]);
        // redirige stdout pour écrire dans pipe_from_cgi
        if (dup2(pipe_from_cgi_[1], STDOUT_FILENO) == -1)
            exit(EXIT_FAILURE);
        // close le fd (car il a été redirigé)
        close(pipe_from_cgi_[1]);

        // prépare les arguments pour execve (l'interpréteur est le nom du programme, et le nom du script est l'argument)
        // exemple : python3 hello.py
        char    *args[] = {
            const_cast<char *>(interpreter.c_str()),
            const_cast<char *>(filePathAbs.c_str()),
            NULL
        };

        // execve
        if (execve(args[0], args, envp) == -1)
        {
            std::cerr << "CGI Error : execve failed : " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // dans le parent
    else if (pid_ > 0)
    {
        // close les pipes qui ne sont plus nécessaires
        close(pipe_to_cgi_[0]);
        close(pipe_from_cgi_[1]);
    }
}

CGISubprocess::~CGISubprocess()
{
    if (this->pipe_to_cgi_[1] != -1)
    {
        close(this->pipe_to_cgi_[1]);
        this->pipe_to_cgi_[1] = -1;
    }
    if (this->pipe_from_cgi_[0] != -1)
    {
        close(this->pipe_from_cgi_[0]);
        this->pipe_to_cgi_[0] = -1;
    }
}

int     CGISubprocess::getWriteFd() const
{
    return (this->pipe_to_cgi_[1]);
}

int     CGISubprocess::getReadFd() const
{
    return (this->pipe_from_cgi_[0]);
}

int     CGISubprocess::getPid() const
{
    return (this->pid_);
}
