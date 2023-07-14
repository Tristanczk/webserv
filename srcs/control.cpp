#include "../includes/webserv.hpp"

extern bool run;
extern std::set<pid_t> pids;

void killChildren() {
	for (std::set<pid_t>::iterator it = pids.begin(); it != pids.end(); ++it) {
		kill(*it, SIGTERM);
	}
}

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
