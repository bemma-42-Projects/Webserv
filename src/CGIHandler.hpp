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

class   CGIHandler
{
    public:
        CGIHandler(Request &request, const std::string &interpreter);
        ~CGIHandler();

        void    execute();

        std::string getBody() const;
        std::string getContentType() const;
        int         getCode() const;

    private:
        char		**getEnvp();
		void		addHeadersToEnv(std::vector<std::string>& env_vector);
		void		parseCgiOutput(const std::string &raw);
        void        freeEnvp(char **envp);

        Request     &request_;
        std::string interpreter_;
        std::string body_;
        std::string content_type_;
        int         code_;
};

#endif