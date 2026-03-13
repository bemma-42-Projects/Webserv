#ifndef WEBSERVEXCEPTION_HPP
# define WEBSERVEXCEPTION_HPP

# include <exception>
# include <string>
# include <cerrno>
# include <cstring>

class WebservException : public std::exception {
    public:
        WebservException(const std::string &message);
        virtual ~WebservException() throw();
        virtual const char *what() const throw();

    private:
        std::string _message;
};

#endif