/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGISubprocess.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 10:14:43 by julien            #+#    #+#             */
/*   Updated: 2026/05/14 14:24:29 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGISubprocess.hpp"
#include "utils.hpp"

#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <sys/wait.h>
#include <string>
#include <iostream>

CGISubprocess::CGISubprocess() {
	pipe_to_cgi_[0] = -1;
	pipe_to_cgi_[1] = -1;
	pipe_from_cgi_[0] = -1;
	pipe_from_cgi_[1] = -1;

	if (pipe(pipe_to_cgi_) != 0)
		throw (std::runtime_error("Failed to create pipe to CGI"));
	setNonBlocking(pipe_to_cgi_[1]);
	if (pipe(pipe_from_cgi_) != 0) {
		close(pipe_to_cgi_[0]);
		close(pipe_to_cgi_[1]);
		throw (std::runtime_error("Failed to create pipe from CGI"));
	}
	setNonBlocking(pipe_from_cgi_[0]);
}

void	CGISubprocess::setupChildPipes_() {
	close(pipe_to_cgi_[1]);
	if (dup2(pipe_to_cgi_[0], STDIN_FILENO) == -1)
		exit(EXIT_FAILURE);
	close(pipe_to_cgi_[0]);
	close(pipe_from_cgi_[0]);
	if (dup2(pipe_from_cgi_[1], STDOUT_FILENO) == -1)
		exit(EXIT_FAILURE);
	close(pipe_from_cgi_[1]);
}

void CGISubprocess::runChild_(const std::string &path, const std::string &interpreter, char **envp) {
    setupChildPipes_();

    std::string parent_dir = ".";
    std::string filename = path;
    size_t      last_slash = path.find_last_of('/');

    if (last_slash != std::string::npos) {
        parent_dir = path.substr(0, last_slash);
        filename = path.substr(last_slash + 1);
    }
    if (chdir(parent_dir.c_str()) == -1) {
        exit(1);
    }
    char *args[] = {
        const_cast<char *>(interpreter.c_str()),
        const_cast<char *>(filename.c_str()),
        NULL
    };
    execve(args[0], args, envp);
    exit(1);
}

void	CGISubprocess::createSubprocess(const std::string &path, const std::string &interpreter, char **envp) {
	this->pid_ = fork();
	if (this->pid_ == -1)
		throw (std::runtime_error("Failed to create fork for CGI"));
	if (this->pid_ == 0)
		runChild_(path, interpreter, envp);
	close(pipe_to_cgi_[0]);
	close(pipe_from_cgi_[1]);
}

CGISubprocess::~CGISubprocess() {
	
}

int	CGISubprocess::getWriteFd() const {
	return (this->pipe_to_cgi_[1]);
}

int	CGISubprocess::getReadFd() const {
	return (this->pipe_from_cgi_[0]);
}

int	CGISubprocess::getPid() const {
	return (this->pid_);
}
