#pragma once
#include <string>

class Config
{
	public:
		Config();
		~Config();
		static void 		setAutoindex(bool value);
		static bool			getAutoindex();
		static void 		setBodySize(size_t value);
		static size_t		getBodySize();
		static void 		setRoot(size_t value);
		static std::string	getRoot();
		static std::string	getIndex();

	private:
		static bool			autoindex_;
		static size_t		body_size_;
		static std::string	root_;
		static std::string	index_;
};
