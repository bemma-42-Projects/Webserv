NAME = webserv

CXX = c++
FLAGS = -Wall -Wextra -Werror -std=c++98 -g 	
OBJDIR = obj

SOURCES = ./Request.cpp ./Config.cpp ./RequestAnswer.cpp ./Location.cpp
OBJS = $(patsubst ./%.cpp,$(OBJDIR)/%.o,$(SOURCES))

all: $(NAME)

$(NAME): $(OBJS)
	@echo "Compilationn...."
	@$(CXX) $(FLAGS) $(OBJS) -o $(NAME)
	@echo "Compilation finished." 

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(OBJDIR)
	@$(CXX) $(FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)
	@echo "clean"

re: fclean $(NAME)

.PHONY: all clean fclean re