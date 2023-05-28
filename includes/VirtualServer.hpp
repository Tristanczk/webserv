#include "Location.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <istream>
#include <map>
#include <sstream>
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
		initKeywordMap();
	}
	~VirtualServer(){};
	void initKeywordMap(void) {
		_keywordHandlers["listen"] = &VirtualServer::parseListen;
		_keywordHandlers["server_name"] = &VirtualServer::parseServerName;
		_keywordHandlers["root"] = &VirtualServer::parseRoot;
		_keywordHandlers["autoindex"] = &VirtualServer::parseAutoIndex;
		_keywordHandlers["client_body_buffer_size"] = &VirtualServer::parseClientBodyBufferSize;
		_keywordHandlers["client_max_body_size"] = &VirtualServer::parseClientMaxBodySize;
		_keywordHandlers["error_page"] = &VirtualServer::parseErrorPage;
		_keywordHandlers["index"] = &VirtualServer::parseIndex;
		_keywordHandlers["location"] = &VirtualServer::parseLocation;
	}
	bool initServer(std::istream& config) {
		std::string line, keyword, val;
		for (; std::getline(config, line);) {
			std::istringstream iss(line);
			if (!(iss >> keyword))
				continue;
			if (keyword == "}")
				return true;
			else if (keyword[0] == '#')
				continue;
			else {
				try {
					KeywordHandler handler = _keywordHandlers.at(keyword);
					if (!(this->*handler)(iss))
						return false;
				} catch (const std::exception& e) {
					std::cerr << "Invalid keyword in configuration file: " << keyword << std::endl;
					return false;
				}
			}
		}
		return true;
	};

	bool parseListen(std::istringstream& iss) {
		std::string value, host;
		int port;
		if (!(iss >> value)) {
			std::cerr << "Missing information after listen keyword" << std::endl;
			return false;
		}
		std::istringstream elements(value);
		// voir comment on gere les valeurs
	}

private:
	typedef bool (VirtualServer::*KeywordHandler)(std::istringstream& iss);
	struct sockaddr_in _address;
	std::string _serverName;
	std::string _rootDir;
	bool _autoIndex; // set to false by default
	std::size_t _bufferSize;
	std::size_t _bodySize;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::vector<Location> _locations;
	std::map< std::string, KeywordHandler> _keywordHandlers;
};
