NAME = webserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRC_DIR = src

SRC = $(SRC_DIR)/parsingconfig.cpp \
		$(SRC_DIR)/LocationConfig.cpp \
		$(SRC_DIR)/ServerConfig.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re