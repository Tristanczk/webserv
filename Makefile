NAME		:= webserv
TEST 		:= webtest

I			:= includes/
O			:= objs/
S			:= srcs/
T			:= tests/

GARBAGE		:= .vscode

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98 -g3 -I$I
VALGRIND	:= valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes -q

vpath %.cpp $S $T

SRCS		:= ${wildcard $S*.cpp}
HEADERS		:= ${wildcard $I*.hpp}

ifeq (test, ${findstring test, ${MAKECMDGOALS}})
	SRCS	:= ${filter-out $Smain.cpp, ${SRCS}}
	SRCS	+= ${wildcard $T*.cpp}
	HEADERS	+= ${wildcard $T*.hpp}
endif

SRCS		:= ${notdir ${SRCS}}
OBJS		:= ${patsubst %.cpp, $O%.o, ${SRCS}}

RESET		:= \033[0m
RED			:= \033[31m
GREEN		:= \033[32m
BLUE		:= \033[34m

all: ${NAME}

${OBJS}: $O%.o: %.cpp ${HEADERS}
	@mkdir -p $O
	@${CXX} ${CXXFLAGS} -c $< -o $@
	@echo "${GREEN}✓ $@${RESET}"

${NAME} ${TEST}: ${OBJS}
	@${CXX} ${CXXFLAGS} $^ -o $@
	@echo "${BLUE}$@ is compiled.${RESET}"

clean:
	rm -rf $O ${GARBAGE}

fclean: clean
	rm -f ${NAME} ${TEST}

re: fclean
	@${MAKE} all

bonus: ${NAME}

test: ${TEST}
	@./${TEST} # TODO launch with valgrind

.PHONY: all bonus clean fclean re test
