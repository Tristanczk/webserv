#pragma once

#include "webserv.hpp"
#include <cstddef>
#include <cstdio>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

class Server {
public:
	Server(){};
	~Server(){};

	bool parseConfig(const char* filename) {
		std::ifstream config(filename);
		if (!config.good()) {
			std::cerr << "Cannot open file " << filename << std::endl;
			return false;
		}
		for (std::string line; std::getline(config, line);) {
			if (line[0] == '#' || line.empty())
				continue;
			if (line == "server {") {
				VirtualServer server;
				if (!server.initServer(config))
					return false;
				_virtualServers.push_back(server);
			} else {
				std::cerr << "Invalid line in config file: " << line << std::endl;
				return false;
			}
		}
		return !_virtualServers.empty() && checkInvalidServers();
	}

	bool initServer() {
		if (!initEpoll())
			return false;
		if (!connectVirtualServers())
			return false;
		return true;
	}

	void loop() {
		int numFds, clientFd;
		while (true) {
			syscall(numFds = epoll_wait(_epollFd, _eventList, MAX_CLIENTS, -1), "epoll_wait");
			for (int i = 0; i < numFds; ++i) {
				if (_eventList[i].data.fd == STDIN_FILENO) {
					std::string message = fullRead(STDIN_FILENO, BUFFER_SIZE_SERVER);
					if (message == "quit\n") {
						std::cout << "Exiting program" << std::endl;
						cleanServer();
						std::exit(EXIT_SUCCESS);
					}
				} else {
					bool check = false;
					for (size_t j = 0; j < _listenSockets.size(); ++j) {
						if (_eventList[i].data.fd == _listenSockets[j]) {
							Client client(&_virtualServers[j]);
							syscall(clientFd = accept(_listenSockets[j],
													  (struct sockaddr*)&client.getAddress(), NULL),
									"accept");
							client.setFd(clientFd);
							syscall(addEpollEvent(clientFd, EPOLLIN), "add epoll event");
							_clients[clientFd] = client;
							check = true;
							break;
						}
					}
					if (!check) {
						clientFd = _eventList[i].data.fd;
					}
				}
			}
		}
	}

	// not sure this function will be useful
	VirtualServer* findMatchingVirtualServer(in_port_t port, in_addr_t addr,
											 const std::string& serverName) {
		int bestMatch = -1;
		t_vsmatch bestMatchLevel = VS_MATCH_NONE;
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			t_vsmatch matchLevel = _virtualServers[i].isMatching(port, addr, serverName);
			if (matchLevel > bestMatchLevel) {
				bestMatch = i;
				bestMatchLevel = matchLevel;
			}
		}
		return bestMatch == -1 ? NULL : &_virtualServers[bestMatch];
	}

	void printVirtualServerList() const {
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			std::cout << "Server " << i << ":" << std::endl;
			_virtualServers[i].printServerInformation();
		}
	}

private:
	std::vector<VirtualServer> _virtualServers;
	int _epollFd;
	std::vector<int> _listenSockets;
	struct epoll_event _eventList[MAX_CLIENTS];
	std::map<int, Client> _clients;

	bool checkInvalidServers() const {
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			for (size_t j = i + 1; j < _virtualServers.size(); ++j) {
				if (_virtualServers[i].getPort() == _virtualServers[j].getPort() &&
					_virtualServers[i].getAddr() == _virtualServers[j].getAddr()) {
					std::vector<std::string> serverNamesI = _virtualServers[i].getServerNames();
					std::vector<std::string> serverNamesJ = _virtualServers[j].getServerNames();
					if (serverNamesI.empty() && serverNamesJ.empty()) {
						std::cerr << "Conflicting server on host:port "
								  << getIpString(_virtualServers[i].getAddr()) << ":"
								  << ntohs(_virtualServers[i].getPort()) << " for server name \"\""
								  << std::endl;
						return false;
					}
					for (size_t k = 0; k < serverNamesI.size(); ++k) {
						for (size_t l = 0; l < serverNamesJ.size(); ++l) {
							if (serverNamesI[k] == serverNamesJ[l]) {
								std::cerr << "Conflicting server on host:port "
										  << getIpString(_virtualServers[i].getAddr()) << ":"
										  << ntohs(_virtualServers[i].getPort())
										  << " for server name: " << serverNamesI[k] << std::endl;
								return false;
							}
						}
					}
				}
			}
		}
		return true;
	}

	bool initEpoll() {
		_epollFd = epoll_create1(0);
		if (_epollFd == -1) {
			std::cerr << "Error when creating epoll" << std::endl;
			return false;
		}
		if (addEpollEvent(STDIN_FILENO, EPOLLIN) == -1)
			return false;
		return true;
	}

	int addEpollEvent(int eventFd, int flags) {
		struct epoll_event event;
		event.data.fd = eventFd;
		event.events = flags;
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, eventFd, &event) == -1) {
			std::cerr << "Error when adding event to epoll" << std::endl;
			return -1;
		}
		return 1;
	}

	bool connectVirtualServers() {
		int socketFd;
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			socketFd = socket(AF_INET, SOCK_STREAM, 0);
			if (socketFd == -1) {
				std::cerr << "Error when creating socket" << std::endl;
				return false;
			}
			if (!bind(socketFd, (struct sockaddr*)&_virtualServers[i].getAddress(),
					  sizeof(_virtualServers[i].getAddress()))) {
				std::cerr << "Error when binding socket" << std::endl;
				return false;
			}
			if (!listen(socketFd, BACKLOG)) {
				std::cerr << "Error when listening on socket" << std::endl;
				return false;
			}
			if (!addEpollEvent(socketFd, EPOLLIN))
				return false;
			_listenSockets.push_back(socketFd);
		}
		return true;
	}

	void cleanServer() {
		for (size_t i = 0; i < _listenSockets.size(); ++i) {
			close(_listenSockets[i]);
		}
		for (size_t i = 0; i < MAX_CLIENTS; ++i) {
			// should we check something on the fd?
			close(_eventList[i].data.fd);
		}
		close(_epollFd);
	}

	void syscall(int returnValue, const char* funcName) {
		if (returnValue == -1) {
			std::perror(funcName);
			cleanServer();
			std::exit(EXIT_FAILURE);
		}
	}
};