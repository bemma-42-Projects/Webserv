NAME = webserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS_FILES = \
			Client.cpp \
			Server.cpp \
			Error.cpp \
			Location.cpp \
			Request.cpp \
			RequestAnswer.cpp \
			Config.cpp \
			main.cpp \

OBJS = $(SRCS_FILES:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re