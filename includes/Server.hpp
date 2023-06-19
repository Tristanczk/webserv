#pragma once

#include "webserv.hpp"
#include <cstddef>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

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
		findVirtualServersToBind();
		if (!initEpoll())
			return false;
		if (!connectVirtualServers())
			return false;
		return true;
	}

	void loop() {
		int numFds, clientFd;
		// std::cout << "Server is running" << std::endl;
		while (true) {
			syscall(numFds = epoll_wait(_epollFd, _eventList, MAX_CLIENTS, -1), "epoll_wait");
			for (int i = 0; i < numFds; ++i) {
				// TO DO : do we keep in the end ?
				if (_eventList[i].data.fd == STDIN_FILENO) {
					std::string message = fullRead(STDIN_FILENO, BUFFER_SIZE_SERVER);
					if (message == "quit\n") {
						std::cout << "Exiting program" << std::endl;
						cleanServer();
						std::exit(EXIT_SUCCESS);
					}
				} else {
					std::map<int, VirtualServer*>::iterator it =
						_listenSockets.find(_eventList[i].data.fd);
					if (it != _listenSockets.end()) {
						Client client(it->second);
						syscall(clientFd = accept(it->first, (struct sockaddr*)&client.getAddress(),
												  &client.getAddressLen()),
								"accept");
						if (!client.setInfo(clientFd)) {
							close(clientFd);
							continue;
						}
						// client.printHostPort();
						syscall(addEpollEvent(clientFd, EPOLLIN | EPOLLET), "add epoll event");
						_clients[clientFd] = client;
						// std::cout << "New client connected" << std::endl;
					} else {
						if (_eventList[i].events & EPOLLIN) {
							clientFd = _eventList[i].data.fd;
							Client& client = _clients[clientFd];
							std::string request = client.readRequest();
							// TO DO : parsing of the request
							// TO DO : building of the response
							if (request.empty()) {
								syscall(removeEpollEvent(clientFd, &_eventList[i]),
										"remove epoll event");
								syscall(close(clientFd), "close");
								_clients.erase(clientFd);
							} else {
								syscall(modifyEpollEvent(clientFd, EPOLLOUT | EPOLLET),
										"modify epoll event");
							}
							// std::cout << "received new request from client" << std::endl;
						} else if (_eventList[i].events & EPOLLOUT) {
							clientFd = _eventList[i].data.fd;
							// TO DO, send response to client
							// int n =
							// 	send(clientFd, HTTP_RESPONSE, strlen(HTTP_RESPONSE), MSG_NOSIGNAL);
							// if (n == -1) {
							// 	std::cerr << "Error while sending response" << std::endl;
							// 	syscall(removeEpollEvent(clientFd, &_eventList[i]),
							// 			"remove epoll event");
							// 	syscall(close(clientFd), "close");
							// 	_clients.erase(clientFd);
							// }
							// std::cout << "sent response to client" << std::endl;
							syscall(modifyEpollEvent(clientFd, EPOLLIN | EPOLLET),
									"modify epoll event");
						}
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

	void printVirtualServerListToBind() const {
		for (size_t i = 0; i < _virtualServersToBind.size(); ++i) {
			std::cout << "Server " << i << ":" << std::endl;
			_virtualServersToBind[i]->printServerInformation();
		}
	}

private:
	std::vector<VirtualServer> _virtualServers;
	std::vector<VirtualServer*> _virtualServersToBind;
	int _epollFd;
	std::map<int, VirtualServer*> _listenSockets;
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
		if (addEpollEvent(STDIN_FILENO, EPOLLIN | EPOLLET) == -1)
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
		return 0;
	}

	int removeEpollEvent(int eventFd, struct epoll_event* event) {
		if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, eventFd, event) == -1) {
			std::cerr << "Error when removing event from epoll" << std::endl;
			return -1;
		}
		return 0;
	}

	int modifyEpollEvent(int eventFd, int flags) {
		struct epoll_event event;
		event.data.fd = eventFd;
		event.events = flags;
		if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, eventFd, &event) == -1) {
			std::cerr << "Error when modifying event in epoll" << std::endl;
			return -1;
		}
		return 0;
	}

	// TODO: need to fix when handling multiple virtual servers on the same host:port because it
	// is not possible to do multiple bind
	void findVirtualServersToBind() {
		in_port_t port;
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			if (_virtualServers[i].getAddr() == INADDR_ANY) {
				// get vector to delete
				// if inaddr_any we delete all that have same port and consider only inaddr_any
				// if not inaddr_any, we push only if no inaddr any in the vector
				std::vector<VirtualServer*>::iterator it = _virtualServersToBind.begin();
				port = _virtualServers[i].getPort();
				while (it != _virtualServersToBind.end()) {
					if (port == (*it)->getPort())
						it = _virtualServersToBind.erase(it);
					else
						++it;
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
				if (!check)
					_virtualServersToBind.push_back(&_virtualServers[i]);
			}
		}
	}

	bool connectVirtualServers() {
		int socketFd;
		for (size_t i = 0; i < _virtualServersToBind.size(); ++i) {
			socketFd = socket(AF_INET, SOCK_STREAM, 0);
			if (socketFd == -1) {
				std::cerr << "Error when creating socket" << std::endl;
				return false;
			}
			int reuse = 1;
			if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
				std::cerr << "Error when setting socket options" << std::endl;
				return false;
			}
			// if (fcntl(socketFd, F_SETFL, O_NONBLOCK) == -1) {
			// 	std::cerr << "Error when setting socket to non-blocking" << std::endl;
			// 	return false;
			// }
			struct sockaddr_in addr = _virtualServersToBind[i]->getAddress();
			if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
				// std::cerr << "Error when binding socket" << std::endl;
				std::perror("bind");
				return false;
			}
			if (listen(socketFd, BACKLOG) == -1) {
				std::cerr << "Error when listening on socket" << std::endl;
				return false;
			}
			if (addEpollEvent(socketFd, EPOLLIN | EPOLLET) == -1)
				return false;
			_listenSockets[socketFd] = _virtualServersToBind[i];
		}
		return true;
	}

	void cleanServer() {
		for (std::map<int, VirtualServer*>::iterator it = _listenSockets.begin();
			 it != _listenSockets.end(); ++it) {
			close(it->first);
		}
		for (size_t i = 0; i < MAX_CLIENTS; ++i) {
			// should we check something on the fd?
			// TODO : need to check in order to avoir valgrind warning on invalid file
			// descriptor in close()
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