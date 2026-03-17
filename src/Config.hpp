#pragma once
#include <string>

class Config
{
	public:
		Config();
		~Config();
		static void setAutoindex(bool value);
		static bool	getAutoindex();

	private:
		static bool autoindex_;
};
