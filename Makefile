NAME		:= webserv

S			:= srcs/
O			:= objs/
I			:= includes/

GARBAGE		:= .vscode

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98 -I$I

SRCS		:= ${wildcard $S*.cpp} ${wildcard $S/*/*.cpp}
HEADERS		:= ${wildcard $I*.hpp}
FILENAMES	:= ${basename ${SRCS}}
FOLDERS 	:= ${sort ${dir ${SRCS}}}
OBJS		:= ${FILENAMES:$S%=$O%.o}

RM			:= rm -rf
MKDIR		:= mkdir -p

END			:= \033[0m
RED			:= \033[31m
GREEN		:= \033[32m
BLUE		:= \033[34m

all: ${NAME}

${OBJS}: $O%.o: $S%.cpp ${HEADERS}
	@${MKDIR} ${FOLDERS:${S}%=${O}%}
	@${CXX} ${CXXFLAGS} -c $< -o $@
	@echo "${GREEN}✓ $@${END}"

${NAME}: ${OBJS}
	@${CXX} ${CXXFLAGS} $^ -o $@ ${CXXLIBS}
	@echo "${BLUE}${NAME} is compiled.${END}"

clean:
	${RM} $O ${GARBAGE}

fclean: clean
	${RM} ${NAME}

re: fclean
	@${MAKE} all

bonus: ${NAME}

.PHONY: all bonus clean fclean re
