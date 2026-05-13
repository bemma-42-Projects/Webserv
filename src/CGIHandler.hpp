/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 16:14:03 by julien            #+#    #+#             */
/*   Updated: 2026/05/13 18:19:51 by julien           ###   ########.fr       */
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
        CGIHandler(Request &request, const std::string &interpreter);
        ~CGIHandler();
        void        execute();
        int         getPid() const;
        int         getReadFd() const;
        int         getWriteFd() const;
        void        appendOutput(const std::string &chunk);
        std::string getRawOutput() const;
        size_t      getBytesSent() const;
        void        handleWrite();

    private:
        std::string getAbsolutePath_() const;
        void        setupStandardEnv_(std::vector<std::string> &env) const;
        char        **vectorToCharArray_(const std::vector<std::string> &env) const;
        char		**getEnvp();
		void		addHeadersToEnv(std::vector<std::string>& env_vector);
        void        freeEnvp(char **envp);

        Request         &request_;
        std::string     interpreter_;
        CGISubprocess   subprocess_;
        std::string     cgi_raw_output_;
        size_t          bytes_sent_;
};

#endif