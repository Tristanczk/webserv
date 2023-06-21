#include "../includes/webserv.hpp"

int addEpollEvent(int epollFd, int eventFd, int flags) {
	struct epoll_event event;
	event.data.fd = eventFd;
	event.events = flags;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, eventFd, &event) == -1) {
		std::perror("EPOLL_CTL_ADD");
		return -1;
	}
	return 0;
}

int modifyEpollEvent(int epollFd, int eventFd, int flags) {
	struct epoll_event event;
	event.data.fd = eventFd;
	event.events = flags;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, eventFd, &event) == -1) {
		std::perror("EPOLL_CTL_MOD");
		return -1;
	}
	return 0;
}

int removeEpollEvent(int epollFd, int eventFd, struct epoll_event* event) {
	if (epoll_ctl(epollFd, EPOLL_CTL_DEL, eventFd, event) == -1) {
		std::perror("EPOLL_CTL_DEL");
		return -1;
	}
	return 0;
}
