#pragma once
#include <string>
#include "Location.hpp"


class Config
{
	public:
		Config();
		~Config();
		//static void 		setAutoindex(bool value);
		//static bool			getAutoindex();
		static void 		setBodySize(size_t value);
		static size_t		getBodySize();
		static void 		setRoot(size_t value);
		static std::string	getRoot();
		//static std::string	getIndex();
		static Location* 	matchLocation(std::string requestPath);
		static void	location();

	private:
		//static bool			autoindex_;
		static size_t		body_size_;
		static std::string	root_;
		//static std::string	index_;
		static std::vector<Location> location_;

};
