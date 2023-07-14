#include "../includes/webserv.hpp"

extern int epollFd;
extern bool run;
extern std::set<pid_t> pids;

static void killChildren() {
	if (pids.empty()) {
		return;
	}
	std::cout << "Killing children... ";
	for (std::set<pid_t>::iterator it = pids.begin(); it != pids.end(); ++it) {
		std::cout << *it << ' ';
		kill(*it, SIGTERM);
	}
	std::cout << '\n';
}

void mainDestructor() {
	killChildren();
	if (epollFd != -1) {
		close(epollFd);
	}
}

void signalHandler(int signum) {
	(void)signum;
	run = false;
}

void syscall(long returnValue, const char* funcName) {
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
