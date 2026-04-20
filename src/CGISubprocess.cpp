/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGISubprocess.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 10:14:43 by julien            #+#    #+#             */
/*   Updated: 2026/04/20 15:35:03 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGISubprocess.hpp"
#include "utils.hpp"

#include <unistd.h>     // Pour pipe(), fork(), dup2(), close(), execve()
#include <cerrno>       // Pour errno
#include <cstring>      // Pour strerror()
#include <cstdlib>      // Pour exit()
#include <stdexcept>    // Pour std::runtime_error
#include <sys/wait.h>   // Pour waitpid
#include <string>
#include <iostream>


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

void    CGISubprocess::setupChildPipes_()
{
    close(pipe_to_cgi_[1]);
    if (dup2(pipe_to_cgi_[0], STDIN_FILENO) == -1)
        exit(EXIT_FAILURE);
    close(pipe_to_cgi_[0]);
    close(pipe_from_cgi_[0]);
    if (dup2(pipe_from_cgi_[1], STDOUT_FILENO) == -1)
        exit(EXIT_FAILURE);
    //if (dup2(pipe_from_cgi_[1], STDERR_FILENO) == -1)
    //    exit(EXIT_FAILURE);
    close(pipe_from_cgi_[1]);
}

void    CGISubprocess::runChild_(const std::string &path, const std::string &interpreter, char **envp)
{
    setupChildPipes_();
    std::string parent_dir = ".";
    size_t      last_slash = path.find_last_of('/');
    if (last_slash != std::string::npos)
        parent_dir = path.substr(0, last_slash);
    if (chdir(parent_dir.c_str()) == -1)
    {
        std::cerr << "CGI Error: chdir failed : " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    char    *args[] = {
        const_cast<char *>(interpreter.c_str()),
        const_cast<char *>(path.c_str()),
        NULL
    };
    if (execve(args[0], args, envp) == -1)
    {
        std::cerr << "CGI Error : execve failed : " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void    CGISubprocess::createSubprocess(const std::string &path, const std::string &interpreter, char **envp)
{
    this->pid_ = fork();
    if (this->pid_ == -1)
        throw (std::runtime_error("Failed to create fork for CGI: " + std::string(strerror(errno))));
    if (this->pid_ == 0)
        runChild_(path, interpreter, envp);
    close(pipe_to_cgi_[0]);
    close(pipe_from_cgi_[1]);
}

// ATTENTION : UTILISER EPOLL A LA PLACE
/*
     * TODO / À AMÉLIORER : 
     * Cette boucle est synchrone/bloquante. Pour un Webserv non-bloquant :
     * 1. Ne pas boucler ici.
     * 2. Ajouter 'pipe_from_cgi_[0]' à l'instance globale d'EPOLL.
     * 3. Laisser la boucle d'événements principale appeler le read quand des données sont prêtes.
     * 4. Gérer un état 'CGI_READING' pour cette connexion.
*/
std::string CGISubprocess::readResponse()
{
    char        buffer[4096];
    std::string response;
    ssize_t     bytes_read;

    while (true)
    {
        bytes_read = read(this->pipe_from_cgi_[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
            response.append(buffer, bytes_read);
        else if (bytes_read == 0)
            break ;
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Temporaire : On attend que le script travaille.
            // En mode EPOLL, on sortirait d'ici pour revenir au main loop.
            usleep(1000);
            continue;
        } else {
            break ;
        }
    }
    close(this->pipe_from_cgi_[0]);
    waitpid(this->pid_, NULL, 0);

    return (response);    
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
