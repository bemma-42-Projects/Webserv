/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGISubprocess.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 10:14:43 by julien            #+#    #+#             */
/*   Updated: 2026/04/28 13:52:24 by julien           ###   ########.fr       */
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


// on construit les pipes d'ecriture et de lecture
// le serveur se dedouble avec fork
// le parent reste le serveur
// l'enfant devient le script CGI
// dup2 permet de rediriger la sortie sur le terminal par une sortie dans le fd d'ecriture
// les echos php iront dans ce pipe
// pendant que le script php fait son travail et remplis le pipe
// le serveur ajoute le fd de lecture dans la boucle epoll
// des que epoll signale qu'il y a des donnees a lire (EPOLLIN), le serveur appelle read sur le fd de lecture
// il ajoute les octets a la chaine avec append raw output
// puis le pipe se ferme
// le prochain read envoie EOF
// puis on appelle build CGI response pour traduire la reponse CGI en reponse HTTP


    // ATTENTION :
    // lorsque l'on crée un pipe, le système donne 2 fd
    // si on appelle fcntl (avec setNonBlocking)
    // le comportement de ce fd est au niveau du kernel
    // puis on fait un fork
    // mais le cgi (processus enfant) recoit une copie exacte des fds du parent !
    // AVEC CES FLAGS !
    // si le pipe est plein, le programme se mettra en pause
    // et attendra que le serveur vide le pipe
    // mais en mode non bloquant, une erreur sera renvoyée directement !
    // EAGAIN
    // un simple script .sh va donc faire crasher le serveur
    // SEUL LE SERVEUR DOIT DONC AVOIR DES FDs NON BLOQUANTS
    // il ne faut donc pas mettre pipe_to_cgi_[0]
    // et pipe_from_cgi_[1] en non blocking

        // direction serveur -> CGI
        // en lecture (pour lire)
        // envoie les donnees au script (0)
        // si le script lit mais que le serveur n'a pas encore tout envoyé
        // le script doit attendre
        // setNonBlocking(pipe_to_cgi_[0]);

    // le serveur va y appeler write pour envoyer des donnees au script (1)
CGISubprocess::CGISubprocess()
{
    pipe_to_cgi_[0] = -1;
    pipe_to_cgi_[1] = -1;
    pipe_from_cgi_[0] = -1;
    pipe_from_cgi_[1] = -1;

    if (pipe(pipe_to_cgi_) != 0)
        throw (std::runtime_error("Failed to create pipe to CGI" + std::string(strerror(errno))));


    setNonBlocking(pipe_to_cgi_[1]);
    if (pipe(pipe_from_cgi_) != 0)
    {
        close(pipe_to_cgi_[0]);
        close(pipe_to_cgi_[1]);
        throw (std::runtime_error("Failed to create pipe from CGI" + std::string(strerror(errno))));
    }
    setNonBlocking(pipe_from_cgi_[0]);
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

// transforme le clone du serveur web en script CGI
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

// fork permet de creer une copie du serveur web
// dans le parent, fork renvoie le PID de l'enfant
// dans l'enfant, il renvoie 0
// -1 : le serveur n'a plus de memoire
// ou limite de processus autorises atteinte
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
        // FIX
        // reset fermer pipe_from_cgi[0]
        // et non pas pipe_to_cgi[0]
        this->pipe_from_cgi_[0] = -1;
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
