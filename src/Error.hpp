#pragma once
#include <string>

class Error 
{
	public:
		Error();
		~Error();
		static void	setError(int error);

	private:
		static int error_;
};
