#include "../includes/webserv.hpp"

// TODO only one function, which accepts int operation

void addEpollEvent(int epollFd, int eventFd, int flags) {
	struct epoll_event event;
	event.data.fd = eventFd;
	event.events = flags;
	syscall(epoll_ctl(epollFd, EPOLL_CTL_ADD, eventFd, &event), "addEpollEvent");
}

void modifyEpollEvent(int epollFd, int eventFd, int flags) {
	struct epoll_event event;
	event.data.fd = eventFd;
	event.events = flags;
	syscall(epoll_ctl(epollFd, EPOLL_CTL_MOD, eventFd, &event), "modifyEpollEvent");
}
