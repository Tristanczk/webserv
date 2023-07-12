#include "../includes/webserv.hpp"

extern bool run;

void signalHandler(int signum) {
	(void)signum;
	run = false;
}

void syscall(int returnValue, const char* funcName) {
	if (returnValue == -1) {
		throw SystemError(funcName);
	}
}

void syscallEpoll(int epollFd, int operation, int eventFd, int flags, const char* opName) {
	struct epoll_event event;
	event.data.fd = eventFd;
	event.events = flags;
	syscall(epoll_ctl(epollFd, operation, eventFd, &event), opName);
}
