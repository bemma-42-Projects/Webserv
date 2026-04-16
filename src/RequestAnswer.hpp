#pragma once
#include <string>
#include "Request.hpp"

class RequestAnswer
{
	public:
		RequestAnswer(Request request);
		~RequestAnswer();
		int			setAnswer();
		int			methodGet();
		int			methodPost();
		void		fullAnswer();
		int			getIfFile(std::string file);
		int			getIfDir();
		std::string	getAnswer();
		int			getError();
		std::string findIndex(Location loc);
		std::string findContentType(const std::string& path);

		bool		isCgi();
		char		**getEnvp();
		void		addHeadersToEnv(std::vector<std::string>& env_vector);
		int			methodCGI();

	private:
		int			code_;//code de sorti ou error
		std::string	content_type_;//type de retour (image txt...)
		std::string	body_;
		int			error_;
		Request		request_;
		std::string	answer_;//ne pas oublier la ligne vide
		std::string	cgi_interpreter_;

};
