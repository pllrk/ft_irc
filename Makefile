NAME = ft_irc
CC = c++
CFLAGS = -Wall -Werror -Wextra -g -std=c++98
SRCS =	main.cpp \
		Client.cpp \
		ServerSetup.cpp \
		ServerClient.cpp \
		ServerChannel.cpp \
		Channel.cpp
		
OBJS = ${SRCS:.cpp=.o}

# Rule to create .o files from .c files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Main target
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)  # Link the object files to create the final binary

clean:
	rm -f ${OBJS}

fclean: clean
	rm -f $(NAME)

re: fclean all  # Rebuild everything

.PHONY: all clean fclean re
