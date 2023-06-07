NAME		:= webserv
TEST 		:= webtest

I			:= includes/
O			:= objs/
S			:= srcs/
T			:= tests/

GARBAGE		:= .vscode

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98 -g3 -I$I
# TODO --track-fds=yes but too annoying on VSCode
VALGRIND	:= valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -q

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

RM			:= rm -rf
MKDIR		:= mkdir -p

END			:= \033[0m
RED			:= \033[31m
GREEN		:= \033[32m
BLUE		:= \033[34m

# lol testlol:
# 	@echo ${SRCS}
# 	@echo ${OBJS}
# 	@echo ${HEADERS}

all: ${NAME}

${OBJS}: $O%.o: %.cpp ${HEADERS}
	@${MKDIR} $O
	@${CXX} ${CXXFLAGS} -c $< -o $@
	@echo "${GREEN}✓ $@${END}"

${NAME} ${TEST}: ${OBJS}
	@${CXX} ${CXXFLAGS} $^ -o $@ ${CXXLIBS}
	@echo "${BLUE}${NAME} is compiled.${END}"

clean:
	${RM} $O ${GARBAGE}

fclean: clean
	${RM} ${NAME}

re: fclean
	@${MAKE} all

bonus: ${NAME}

test: ${TEST}
	@./${TEST} # TODO launch with Valgrind

.PHONY: all bonus clean fclean re test
