#include "../includes/webserv.hpp"

extern bool run;

void signalHandler(int signum) {
	(void)signum;
	run = false;
}

void syscall(int returnValue, const char* funcName) {
	if (returnValue == -1) {
		std::perror(funcName);
		throw SystemError(funcName);
	}
}
