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
		void		methodDelete();
		void		fullAnswer();
		int			getIfFile(std::string file);
		int			getIfDir();
		std::string	getAnswer();
		int			getError();
		std::string findIndex(LocationConfig loc);
		std::string findContentType(const std::string& path);
		int			fileName();
		std::string Itoa(int nbr);


	private:
		LocationConfig	loc_;
		int				code_;//code de sorti ou error
		std::string 	message_;
		std::string		content_type_;//type de retour (image txt...)
		std::string		body_;
		//int				error_;
		Request			request_;
		std::string		answer_;//ne pas oublier la ligne vide
		std::string		post_file_name_;

};
