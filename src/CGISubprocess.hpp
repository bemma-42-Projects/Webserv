/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGISubprocess.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 10:15:03 by julien            #+#    #+#             */
/*   Updated: 2026/04/20 15:43:50 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGISUBPROCESS_HPP
#define CGISUBPROCESS_HPP

#include <unistd.h>
#include <string>

class   CGISubprocess
{
    public:
        CGISubprocess();
        ~CGISubprocess();
        void        createSubprocess(const std::string &path, const std::string &interpreter, char **envp);
        int         getWriteFd() const;
        int         getReadFd() const;
        pid_t       getPid() const;
        std::string readResponse();

    private:
        void    setupChildPipes_();
        void    runChild_(const std::string &path, const std::string &interpreter, char **envp);
        
        int     pipe_to_cgi_[2];
        int     pipe_from_cgi_[2];
        pid_t   pid_;

        CGISubprocess(const CGISubprocess &src);
        CGISubprocess   &operator=(const CGISubprocess &rhs);
};

#endif
