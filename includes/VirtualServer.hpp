#include "Location.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE_SERVER 16384
#define BUFFER_SIZE_CONF 1024
#define BODY_SIZE 1048576
#define DEFAULT_ERROR 0

class VirtualServer {
public:
	VirtualServer() {
		std::memset(&_address, 0, sizeof(_address));
		_address.sin_family = AF_INET;
		_address.sin_port = htons(DEFAULT_PORT);
		_serverName = "";
		_rootDir = "./www/";
		_autoIndex = false;
		_bufferSize = BUFFER_SIZE_SERVER;
		_bodySize = BODY_SIZE;
		_errorPages[DEFAULT_ERROR] = "./www/default_error.html";
	}
	~VirtualServer(){};

private:
	struct sockaddr_in _address;
	std::string _serverName;
	std::string _rootDir;
	bool _autoIndex; // set to false by default
	std::size_t _bufferSize;
	std::size_t _bodySize;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::vector<Location> _locations;
};
