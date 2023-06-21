#pragma once

#include "webserv.hpp"

class Server {
public:
	Server(){};
	~Server(){};

	bool init(const char* filename) {
		if (!parseConfig(filename))
			return false;
		findVirtualServersToBind();
		return initEpoll() && connectVirtualServers();
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
			if (line[0] == '#' || line.empty())
				continue;
			if (line == "server {") {
				VirtualServer vs;
				if (!vs.init(config))
					return false;
				_virtualServers.push_back(vs);
			} else
				return configFileError("invalid line in config file: " + line);
		}
		if (_virtualServers.empty())
			return configFileError("no server found in " + std::string(filename));
		return checkDuplicateServers();
	}

	void loop() {
		int numFds, clientFd;
		// std::cout << "Server is running" << std::endl;
		while (true) {
			syscall(numFds = epoll_wait(_epollFd, _eventList, MAX_CLIENTS, -1), "epoll_wait");
			for (int i = 0; i < numFds; ++i) {
				// TODO : do we keep in the end ?
				if (_eventList[i].data.fd == STDIN_FILENO) {
					std::string message = fullRead(STDIN_FILENO, BUFFER_SIZE_SERVER);
					if (message == "quit\n") {
						std::cout << "Exiting program" << std::endl;
						// TODO return and let the main call ~Server()
						cleanServer(numFds);
						std::exit(EXIT_SUCCESS);
					}
				} else {
					std::set<int>::iterator it = _listenSockets.find(_eventList[i].data.fd);
					if (it != _listenSockets.end()) {
						Client client;
						syscall(clientFd = accept(*it, (struct sockaddr*)&client.getAddress(),
												  &client.getAddressLen()),
								"accept", numFds);
						if (!client.setInfo(clientFd)) {
							close(clientFd);
							continue;
						}
						client.findAssociatedServers(_virtualServers);
						// client.printHostPort();
						syscall(addEpollEvent(_epollFd, clientFd, EPOLLIN | EPOLLET | EPOLLRDHUP),
								"add epoll event", numFds);
						_clients[clientFd] = client;
						// std::cout << "New client connected" << std::endl;
					} else {
						// should we handle error or unexpected closure differently ?
						// especially is there a need to handle EPOLLRDHUP in a specific way ?
						// because it might indicate that the connection is only half closed and can
						// still receive data information from the server
						if (_eventList[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
							syscall(close(clientFd), "close", numFds);
							_clients.erase(clientFd);
							continue;
						} else if (_eventList[i].events & EPOLLIN) {
							clientFd = _eventList[i].data.fd;
							Client& client = _clients[clientFd];
							std::string request = client.readRequest();
							// TODO : parsing of the request
							// TODO : get the server name in order to find the best matching server
							// for the request
							// std::string serverName =
							// client.findServerName(request); VirtualServer* server =
							// client.findBestMatch(serverName); TODO : building of the response
							if (request.empty()) {
								syscall(close(clientFd), "close", numFds);
								_clients.erase(clientFd);
							} else {
								// TODO: check if the request is complete before swapping to
								// EPOLLOUT
								syscall(modifyEpollEvent(_epollFd, clientFd,
														 EPOLLOUT | EPOLLET | EPOLLRDHUP),
										"modify epoll event", numFds);
							}
							// std::cout << "received new request from client" << std::endl;
						} else if (_eventList[i].events & EPOLLOUT) {
							clientFd = _eventList[i].data.fd;
							// TODO, send response to client
							// int n =
							// 	send(clientFd, HTTP_RESPONSE, strlen(HTTP_RESPONSE), MSG_NOSIGNAL);
							// if (n == -1) {
							// 	std::cerr << "Error while sending response" << std::endl;
							// 	syscall(close(clientFd), "close");
							// 	_clients.erase(clientFd);
							// }
							// std::cout << "sent response to client" << std::endl;
							syscall(modifyEpollEvent(_epollFd, clientFd,
													 EPOLLIN | EPOLLET | EPOLLRDHUP),
									"modify epoll event", numFds);
						}
					}
				}
			}
		}
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
	std::set<int> _listenSockets;
	struct epoll_event _eventList[MAX_CLIENTS];
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
					if (serverNamesI.empty() && serverNamesJ.empty())
						conflict = "\"\"";
					else {
						const std::string* commonServerName =
							findCommonString(serverNamesI, serverNamesJ);
						if (!commonServerName)
							continue;
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

	bool initEpoll() {
		_epollFd = epoll_create1(0);
		if (_epollFd == -1) {
			std::cerr << "Error when creating epoll" << std::endl;
			return false;
		}
		if (addEpollEvent(_epollFd, STDIN_FILENO, EPOLLIN | EPOLLET) == -1)
			return false;
		return true;
	}

	void findVirtualServersToBind() {
		in_port_t port;
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			if (_virtualServers[i].getAddr() == INADDR_ANY) {
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
		for (size_t i = 0; i < _virtualServersToBind.size(); ++i) {
			int socketFd = socket(AF_INET, SOCK_STREAM, 0);
			if (socketFd == -1) {
				std::cerr << "Error when creating socket" << std::endl;
				return false;
			}
			int reuse = 1;
			if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
				std::cerr << "Error when setting socket options" << std::endl;
				return false;
			}
			struct sockaddr_in addr = _virtualServersToBind[i]->getAddress();
			if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
				std::perror("bind");
				return false;
			}
			if (listen(socketFd, SOMAXCONN) == -1) {
				std::cerr << "Error when listening on socket" << std::endl;
				return false;
			}
			if (addEpollEvent(_epollFd, socketFd, EPOLLIN | EPOLLET) == -1)
				return false;
			_listenSockets.insert(socketFd);
		}
		return true;
	}

	void cleanServer(size_t eventsNb = 0) {
		for (std::set<int>::iterator it = _listenSockets.begin(); it != _listenSockets.end();
			 ++it) {
			close(*it);
		}
		for (size_t i = 0; i < eventsNb; ++i) {
			if (_eventList[i].data.fd != STDIN_FILENO) {
				close(_eventList[i].data.fd);
			}
		}
		close(_epollFd);
	}

	void syscall(int returnValue, const char* funcName, size_t eventsNb = 0) {
		if (returnValue == -1) {
			std::perror(funcName);
			cleanServer(eventsNb);
			std::exit(EXIT_FAILURE);
		}
	}
};