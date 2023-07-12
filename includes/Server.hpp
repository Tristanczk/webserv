#pragma once

#include "webserv.hpp"
#include <cstdio>
#include <sys/epoll.h>

extern bool run;

class Server {
public:
	Server() : _numFds(0) {
		std::memset(_eventList, 0, sizeof(_eventList));
		syscall(_epollFd = epoll_create1(0), "epoll_create1");
	};

	~Server() {
		for (std::set<int>::iterator it = _listenSockets.begin(); it != _listenSockets.end();
			 ++it) {
			close(*it);
		}
		for (int i = 0; i < _numFds; ++i) {
			close(_eventList[i].data.fd);
		}
		if (_epollFd != -1) {
			close(_epollFd);
		}
	};

	bool init(const char* filename) {
		if (!parseConfig(filename)) {
			return false;
		}
		findVirtualServersToBind();
		connectVirtualServers();
		return true;
	}

	bool parseConfig(const char* filename) {
		if (isDirectory(filename)) {
			std::cerr << filename << " is a directory" << std::endl;
			return false;
		}
		std::ifstream config(filename);
		if (!config.good()) {
			std::cerr << "Cannot open file " << filename << std::endl;
			return false;
		}
		for (std::string line; std::getline(config, line);) {
			if (line[0] == '#' || line.empty()) {
				continue;
			}
			if (line == "server {") {
				VirtualServer vs;
				if (!vs.init(config)) {
					return false;
				}
				_virtualServers.push_back(vs);
			} else {
				return configFileError("invalid line in config file: " + line);
			}
		}
		if (_virtualServers.empty()) {
			return configFileError("no server found in " + std::string(filename));
		}
		return checkDuplicateServers();
	}

	void loop() {
		while (run) {
			_numFds = epoll_wait(_epollFd, _eventList, MAX_EVENTS, -1);
			if (_numFds < 0) {
				if (!run) {
					break;
				}
				throw SystemError("epoll_wait");
			}
			for (int i = 0; i < _numFds; ++i) {
				std::set<int>::iterator it = _listenSockets.find(_eventList[i].data.fd);
				if (it != _listenSockets.end()) {
					Client client;
					int clientFd = accept(*it, (struct sockaddr*)&client.getAddress(),
										  &client.getAddressLen());
					if (clientFd < 0) {
						std::cerr << RED << "accept: " << std::strerror(errno) << RESET
								  << std::endl;
						continue;
					}
					client.setInfo(clientFd);
					client.findAssociatedServers(_virtualServers);
					syscallEpoll(_epollFd, EPOLL_CTL_ADD, clientFd, EPOLLIN | EPOLLRDHUP,
								 "EPOLL_CTL_ADD");
					_clients[clientFd] = client;
				} else {
					int clientFd = _eventList[i].data.fd;
					Client& client = _clients[clientFd];
					ResponseStatusEnum status;
					if (_eventList[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
						close(clientFd);
						_clients.erase(clientFd);
					} else if (_eventList[i].events & EPOLLIN) {
						status = client.handleRequest();
						if (status == RESPONSE_FAILURE) {
							close(clientFd);
							_clients.erase(clientFd);
						} else if (status == RESPONSE_SUCCESS) {
							syscallEpoll(_epollFd, EPOLL_CTL_MOD, clientFd, EPOLLOUT | EPOLLRDHUP,
										 "EPOLL_CTL_MOD");
						}
					} else if (_eventList[i].events & EPOLLOUT) {
						status = client.pushResponse();
						if (status == RESPONSE_FAILURE) {
							close(clientFd);
							_clients.erase(clientFd);
						} else if (status == RESPONSE_SUCCESS) {
							syscallEpoll(_epollFd, EPOLL_CTL_MOD, clientFd, EPOLLIN | EPOLLRDHUP,
										 "EPOLL_CTL_MOD");
						}
					}
				}
			}
		}
	}

	std::vector<VirtualServer>& getVirtualServers() { return _virtualServers; }

private:
	std::vector<VirtualServer> _virtualServers;
	std::vector<VirtualServer*> _virtualServersToBind;
	int _epollFd;
	int _numFds;
	std::set<int> _listenSockets;
	struct epoll_event _eventList[MAX_EVENTS];
	std::map<int, Client> _clients;

	bool checkDuplicateServers() const {
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			for (size_t j = i + 1; j < _virtualServers.size(); ++j) {
				if (_virtualServers[i].getPort() == _virtualServers[j].getPort() &&
					_virtualServers[i].getAddr() == _virtualServers[j].getAddr()) {
					const std::vector<std::string>& serverNamesI =
						_virtualServers[i].getServerNames();
					const std::vector<std::string>& serverNamesJ =
						_virtualServers[j].getServerNames();
					std::string conflict;
					if (serverNamesI.empty() && serverNamesJ.empty()) {
						conflict = "\"\"";
					} else {
						const std::string* commonServerName =
							findCommonString(serverNamesI, serverNamesJ);
						if (!commonServerName) {
							continue;
						}
						conflict = *commonServerName;
					}
					return configFileError("conflicting server on " +
										   getIpString(_virtualServers[i].getAddr()) + ":" +
										   toString(ntohs(_virtualServers[i].getPort())) +
										   " for server name: " + conflict);
				}
			}
		}
		return true;
	}

	void findVirtualServersToBind() {
		in_port_t port;
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			if (_virtualServers[i].getAddr() == INADDR_ANY) {
				std::vector<VirtualServer*>::iterator it = _virtualServersToBind.begin();
				port = _virtualServers[i].getPort();
				while (it != _virtualServersToBind.end()) {
					if (port == (*it)->getPort()) {
						it = _virtualServersToBind.erase(it);
					} else {
						++it;
					}
				}
				_virtualServersToBind.push_back(&_virtualServers[i]);
			} else {
				bool check = false;
				port = _virtualServers[i].getPort();
				in_addr_t addr = _virtualServers[i].getAddr();
				for (size_t j = 0; j < _virtualServersToBind.size(); ++j) {
					if (_virtualServers[j].getPort() == port &&
						(_virtualServersToBind[j]->getAddr() == INADDR_ANY ||
						 _virtualServersToBind[j]->getAddr() == addr)) {
						check = true;
						break;
					}
				}
				if (!check) {
					_virtualServersToBind.push_back(&_virtualServers[i]);
				}
			}
		}
	}

	void connectVirtualServers() {
		int reuse = 1;
		for (size_t i = 0; i < _virtualServersToBind.size(); ++i) {
			int socketFd;
			syscall(socketFd = socket(AF_INET, SOCK_STREAM, 0), "socket");
			syscall(setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)),
					"setsockopt");
			struct sockaddr_in addr = _virtualServersToBind[i]->getAddress();
			std::cout << BLUE << "Listening on port " << htons(addr.sin_port) << ". 👂\n" << RESET;
			syscall(bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)), "bind");
			syscall(listen(socketFd, SOMAXCONN), "listen");
			syscallEpoll(_epollFd, EPOLL_CTL_ADD, socketFd, EPOLLIN, "EPOLL_CTL_ADD");
			_listenSockets.insert(socketFd);
		}
	}
};