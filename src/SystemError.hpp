#ifndef SYSTEMERROR_HPP
# define SYSTEMERROR_HPP

# include <exception>
# include <string>
# include <cerrno>
# include <cstring>

class SystemError : public std::exception {
	public:
		SystemError(const std::string &message);
		virtual				~SystemError() throw();
		virtual const char	*what() const throw();

	private:
		std::string			_message;
};

#endif
