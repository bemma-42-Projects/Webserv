#include "Error.hpp"
#include <sstream>
#include "Config.hpp"
#include <fcntl.h>    // pour open
#include <unistd.h> 
#include <sys/stat.h>

Error::Error(){}

Error::~Error(){}

int         Error::code_ = 0;
std::string Error::message_ = "";
Location*	Error::loc_ = NULL;

std::string Error::Itoa(int nbr)
{
	std::stringstream ss;
    
    ss << nbr;
    std::string str = ss.str();
	return str;
}

std::string	Error::ErrorPage()
{
	if (loc_ != NULL)
	{
		std::map<int, std::string>	error_loc = loc_->getError();
		//if (error_conf.find(400) != error_conf.end())
		std::map<int, std::string>::const_iterator it = error_loc.find(code_);
		if (it != error_loc.end())
		{
			struct stat s;
			if (stat(it->second.c_str(), &s) == 0 && S_ISREG(s.st_mode))
			{
				int	fd = open((it->second).c_str(), O_RDONLY);
				if (fd != -1)
				{
					std::string	res;
					char buffer[4096];//taille de la reponse ([4096])
					ssize_t	bite_read;
					while ((bite_read = read(fd, buffer, sizeof(buffer))) > 0)
					{
						res.append(buffer, bite_read);
					}
					close(fd);
				
					return (res);
				}
			}	
		}

	}
	
	std::map<int, std::string>	error_conf = Config::getError();
	//if (error_conf.find(400) != error_conf.end())
	std::map<int, std::string>::const_iterator itr = error_conf.find(code_);
	if (itr != error_conf.end())
	{
		struct stat s;
		if (stat(itr->second.c_str(), &s) == 0 && S_ISREG(s.st_mode))
		{
			int	fd = open((itr->second).c_str(), O_RDONLY);
			if (fd != -1)
			{
				std::string	res;
				char buffer[4096];//taille de la reponse ([4096])
				ssize_t	bite_read;
				while ((bite_read = read(fd, buffer, sizeof(buffer))) > 0)
				{
					res.append(buffer, bite_read);
				}
				close(fd);
		
				return (res);
			}
		}	
	}
	std::string	code_str = Itoa(code_);
	return ("<html>"
		"<head><title>" + code_str + " " + message_ + " </title></head>"
		"<body>"
		"<center><h1>" + code_str + " " + message_ + " </h1></center>"
		"<hr><center>Webserv/1.0</center>"
		"</body>"
		"</html>");
}


std::string	Error::AnswerError(int code, std::string message, Location* loc)
{
	code_ = code;
	message_ = message;
	loc_ = loc;
	//(void)loc_;
	std::string error_page = ErrorPage();
	std::string header = "HTTP/1.1 " + Itoa(code_);
	header += " " + message_ + "\r\n";
	std::string content_type = "text/html";

	header += "Content-Type: " + content_type + "\r\n";
	header += "Content-Length: " + Itoa(error_page.length()) + "\r\n";
	header += "\r\n";

	//std::cout << "header = " << header << std::endl;

	//answer_error = header + error_page;
	return (header + error_page);
}