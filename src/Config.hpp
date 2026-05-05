#pragma once
#include <string>
#include <map>
#include "LocationConfig.hpp"


class Config
{
	public:
		Config();
		~Config();
		//static void 			setAutoindex(bool value);
		//static bool			getAutoindex();
		static void 		setBodySize(size_t value);
		static size_t		getBodySize();
		static void 		setRoot(size_t value);
		static std::string	getRoot();
		static std::map<int, std::string>	getError();
		//static std::vector<std::string>	getIndex();
		//static LocationConfig* 	matchLocation(std::string requestPath);
		static void	location();

	private:
		//static bool			autoindex_;
		static size_t						body_size_;
		static std::string					root_;
		//static std::vector<std::string>		index_; 
		static std::vector<LocationConfig> 		location_;
		static std::map<int, std::string>	error_;

};
