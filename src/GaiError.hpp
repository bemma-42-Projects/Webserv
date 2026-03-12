#ifndef GAIERROR_HPP
# define GAIERROR_HPP

# include <exception>
# include <string>

class GaiError : public std::exception {
    public:
        GaiError(const std::string &context, int errcode);
        virtual ~GaiError() throw();
        virtual const char *what() const throw();

    private:
        std::string _message;
};

#endif