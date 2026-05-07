#pragma once
#include <string>
#include "Request.hpp"
#include "CGISubprocess.hpp"

#include "CGIHandler.hpp"

enum	AnswerStatus
{
	ERROR = 0,
	READY_TO_SEND = 1,
	CGI_IN_PROGRESS = 2
};

class RequestAnswer
{
	public:
		RequestAnswer();
		RequestAnswer(const RequestAnswer &src);
		RequestAnswer			&operator=(const RequestAnswer &rhs);
		~RequestAnswer();
		void					setCode(int code);
		void					setMessage(const std::string &message);
		AnswerStatus			setAnswer(Request &request);
		AnswerStatus			methodGet();
		AnswerStatus			methodPost();
		AnswerStatus			methodDelete();
		void					fullAnswer();
		AnswerStatus			getIfFile(std::string file);
		AnswerStatus			getIfDir();
		const std::string		&getAnswer() const;
		int						getError() const;
		CGIHandler				*getCGIHandler() const;
		std::string 			findIndex(LocationConfig loc);
		std::string 			findContentType(const std::string& path);
		int						fileName();
		bool					isCgi();
		AnswerStatus			methodCGI();
		void					buildCGIResponse();
		bool					isResponseFullySent() const;
		void					eraseSentBytes(size_t bytes_sent);
		void					clear();		std::string Itoa(int nbr);
		void					setFullAnswer(const std::string& full_response);

	private:
		LocationConfig	loc_;
		int				code_;
		std::string 	message_;
		int				error_;
		Request			*request_;
		CGIHandler		*cgi_handler_;
		std::string		answer_;
		std::string		content_type_;
		std::string		body_;		
		std::string		post_file_name_;
		std::string		cgi_interpreter_;
};
