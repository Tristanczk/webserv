#pragma once

#include "webserv.hpp"

extern bool run;

class Server {
public:
	Server() {
		initMimeTypes();
		syscall(_epollFd = epoll_create1(0), "epoll_create1");
	};

	~Server() {
		for (std::set<int>::iterator it = _listenSockets.begin(); it != _listenSockets.end();
			 ++it) {
			close(*it);
		}
		for (int i = 0; i < _numFds; ++i) {
			if (_eventList[i].data.fd != STDIN_FILENO) {
				close(_eventList[i].data.fd);
			}
		}
		if (_epollFd != -1)
			close(_epollFd);
	};

	bool init(const char* filename) {
		if (!parseConfig(filename))
			return false;
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
		int clientFd;
		while (run) {
			_numFds = epoll_wait(_epollFd, _eventList, MAX_CLIENTS, -1);
			if (_numFds < 0) {
				if (!run)
					break;
				throw SystemError("epoll_wait");
			}
			for (int i = 0; i < _numFds; ++i) {
				std::set<int>::iterator it = _listenSockets.find(_eventList[i].data.fd);
				if (it != _listenSockets.end()) {
					Client client;
					// TODO : should we exit the server in case of accept error ?
					syscall(clientFd = accept(*it, (struct sockaddr*)&client.getAddress(),
											  &client.getAddressLen()),
							"accept");
					client.setInfo(clientFd);
					client.findAssociatedServers(_virtualServers);
					addEpollEvent(_epollFd, clientFd, EPOLLIN | EPOLLRDHUP);
					_clients[clientFd] = client;
					// std::cout << "New client connected" << std::endl;
				} else {
					// should we handle error or unexpected closure differently ?
					// especially is there a need to handle EPOLLRDHUP in a specific way ?
					// because it might indicate that the connection is only half closed and can
					// still receive data information from the server
					if (_eventList[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
						// ??? Il sort d'ou ce clientFd?
						syscall(close(clientFd), "close");
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
							syscall(close(clientFd), "close");
							_clients.erase(clientFd);
						} else {
							// TODO: check if the request is complete before swapping to
							// EPOLLOUT
							modifyEpollEvent(_epollFd, clientFd, EPOLLOUT | EPOLLRDHUP);
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
						modifyEpollEvent(_epollFd, clientFd, EPOLLIN | EPOLLRDHUP);
					}
				}
			}
		}
	}

private:
	std::vector<VirtualServer> _virtualServers;
	std::vector<VirtualServer*> _virtualServersToBind;
	int _epollFd;
	int _numFds;
	std::set<int> _listenSockets;
	struct epoll_event _eventList[MAX_CLIENTS];
	std::map<int, Client> _clients;
	std::map<std::string, std::string> _mimeTypes;

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

	void initMimeTypes() {
		_mimeTypes["css"] = "text/css";
		_mimeTypes["html"] = "text/html";
		_mimeTypes["jpg"] = "image/jpeg";
		_mimeTypes["js"] = "text/javascript";
		_mimeTypes["mp4"] = "video/mp4";
		_mimeTypes["png"] = "image/png";
		_mimeTypes["svg"] = "image/svg+xml";

		std::ifstream ifs(PATH_MIME_TYPES);
		std::string line, mime, extension;
		while (true) {
			if (!std::getline(ifs, line))
				return;
			std::stringstream lineStream(line);
			if (line.empty() || line[0] == '#' || !(lineStream >> mime))
				continue;
			while (lineStream >> extension)
				_mimeTypes[extension] = mime;
		}
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

	void connectVirtualServers() {
		int reuse = 1;
		for (size_t i = 0; i < _virtualServersToBind.size(); ++i) {
			int socketFd;
			syscall(socketFd = socket(AF_INET, SOCK_STREAM, 0), "socket");
			syscall(setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)),
					"setsockopt");
			struct sockaddr_in addr = _virtualServersToBind[i]->getAddress();
			syscall(bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)), "bind");
			syscall(listen(socketFd, SOMAXCONN), "listen");
			addEpollEvent(_epollFd, socketFd, EPOLLIN);
			_listenSockets.insert(socketFd);
		}
	}

public:
	static void printVirtualServers(const std::vector<VirtualServer>& virtualServers) {
		for (size_t i = 0; i < virtualServers.size(); ++i) {
			std::cout << "Server " << i << ":" << std::endl;
			virtualServers[i].print();
		}
	}
};