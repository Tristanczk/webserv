#pragma once

#include "Location.hpp"
#include <arpa/inet.h>
#include <climits>
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
#include <stdlib.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE_SERVER 16384
#define BUFFER_SIZE_SERVER_LIMIT 1048576 // 1MB
#define BUFFER_SIZE_CONF 1024
#define BODY_SIZE 1048576
#define BODY_SIZE_LIMIT 1073741824 // 1GB
#define DEFAULT_ERROR 0

class VirtualServer {

public:
	VirtualServer() {
		std::memset(&_address, 0, sizeof(_address));
		_address.sin_family = AF_INET;
		_address.sin_port = htons(DEFAULT_PORT);
		_address.sin_addr.s_addr = htonl(INADDR_ANY);
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
		_keywordHandlers["server_name"] = &VirtualServer::parseServerNames;
		_keywordHandlers["root"] = &VirtualServer::parseRoot;
		_keywordHandlers["autoindex"] = &VirtualServer::parseAutoIndex;
		_keywordHandlers["client_body_buffer_size"] = &VirtualServer::parseClientBodyBufferSize;
		_keywordHandlers["client_max_body_size"] = &VirtualServer::parseClientMaxBodySize;
		_keywordHandlers["error_page"] = &VirtualServer::parseErrorPages;
		_keywordHandlers["index"] = &VirtualServer::parseIndex;
		// _keywordHandlers["location"] = &VirtualServer::parseLocation;
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

	void	printServerInformation(void) {
		std::cout << "Server information:" << std::endl;
		std::cout << "Address: " << inet_ntoa(_address.sin_addr) << std::endl;
		std::cout << "Port: " << ntohs(_address.sin_port) << std::endl;
		std::cout << "Server names: ";
		for (std::vector<std::string>::iterator it = _serverNames.begin(); it != _serverNames.end(); it++)
			std::cout << *it << ", ";
		std::cout << std::endl;
		std::cout << "Root directory: " << _rootDir << std::endl;
		std::cout << "Autoindex: " << (_autoIndex ? "on" : "off") << std::endl;
		std::cout << "Client body buffer size: " << _bufferSize << std::endl;
		std::cout << "Client max body size: " << _bodySize << std::endl;
		std::cout << "Error pages:" << std::endl;
		for (std::map<int, std::string>::iterator it = _errorPages.begin(); it != _errorPages.end(); it++)
			std::cout << "error " << it->first << ": " << it->second << std::endl;
		std::cout << "Index pages: ";
		for (std::vector<std::string>::iterator it = _indexPages.begin(); it != _indexPages.end(); it++)
			std::cout << *it << ", ";
		std::cout << std::endl;
		// std::cout << "Locations:" << std::endl;
		// for (std::vector<Location>::iterator it = _locations.begin(); it != _locations.end(); it++)
		// 	it->printLocationInformation();
	}

	

private:
	typedef bool (VirtualServer::*KeywordHandler)(std::istringstream& iss);
	struct sockaddr_in _address;
	std::vector<std::string> _serverNames;
	std::string _rootDir;
	bool _autoIndex; // set to false by default
	std::size_t _bufferSize;
	std::size_t _bodySize;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::vector<Location> _locations;
	std::map< std::string, KeywordHandler> _keywordHandlers;

	bool parseListen(std::istringstream& iss) {
		(void)iss;
		// std::string value, host;
		// int port;
		// if (!(iss >> value)) {
		// 	std::cerr << "Missing information after listen keyword" << std::endl;
		// 	return false;
		// }
		// std::istringstream elements(value);
		// voir comment on gere les valeurs
		return true;
	}

	bool parseServerNames(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << "Missing information after server_name keyword" << std::endl;
			return false;
		}
		_serverNames.push_back(value);
		while (iss >> value)
			_serverNames.push_back(value);
		return true;
	}

	bool parseRoot(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << "Missing information after root keyword" << std::endl;
			return false;
		}
		_rootDir = value;
		if (iss >> value) {
			std::cerr << "Too many arguments after root keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseAutoIndex(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << "Missing information after autoindex keyword" << std::endl;
			return false;
		}
		if (value == "on")
			_autoIndex = true;
		else if (value == "off")
			_autoIndex = false;
		else {
			std::cerr << "Invalid value for autoindex keyword: " << value << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << "Too many arguments after autoindex keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseClientBodyBufferSize(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << "Missing information after client_body_buffer_size keyword" << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			std::cerr << "Invalid character for client_body_buffer_size" << std::endl;
			return false;
		}
		_bufferSize = std::strtol(value.c_str(), NULL, 10);
		if (_bufferSize == LONG_MAX) {
			std::cerr << "Invalid value for client_body_buffer_size" << std::endl;
			return false;
		}
		if (value[idx] != '\0') {
			switch (std::tolower(value[idx]))
			{
			case 'k':
				_bufferSize *= 1024;
				break;
			case 'm':
				_bufferSize *= 1048576;
				break;
			// case 'g':
			// 	_bufferSize *= 1073741824;
			// 	break;
			// we do not accept gigabytes as the size would be too large
			default:
				std::cerr << "Invalid suffix for bytes value, valid suffix are: k, K, m, M" << std::endl;
				return false;
			}
			if (value[idx + 1] != '\0') {
				std::cerr << "Invalid character after suffix for bytes value" << std::endl;
				return false;
			}
		}
		if (_bufferSize > BUFFER_SIZE_SERVER_LIMIT) {
			std::cerr << "Buffer size too big, maximum is: " << BUFFER_SIZE_SERVER_LIMIT << " bytes" << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << "Too many arguments after client_body_buffer_size keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseClientMaxBodySize(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << "Missing information after client_body_buffer_size keyword" << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			std::cerr << "Invalid character for client_body_buffer_size" << std::endl;
			return false;
		}
		_bodySize = std::strtol(value.c_str(), NULL, 10);
		if (_bodySize == LONG_MAX) {
			std::cerr << "Invalid value for client_body_buffer_size" << std::endl;
			return false;
		}
		if (value[idx] != '\0') {
			switch (std::tolower(value[idx]))
			{
			case 'k':
				_bodySize *= 1024;
				break;
			case 'm':
				_bodySize *= 1048576;
				break;
			case 'g':
				_bodySize *= 1073741824;
				break;
			default:
				std::cerr << "Invalid suffix for bytes value, valid suffix are: k, K, m, M, g, G" << std::endl;
				return false;
			}
			if (value[idx + 1] != '\0') {
				std::cerr << "Invalid character after suffix for bytes value" << std::endl;
				return false;
			}
		}
		if (_bodySize > BODY_SIZE_LIMIT) {
			std::cerr << "Body size too big, maximum is: " << BODY_SIZE_LIMIT << " bytes" << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << "Too many arguments after client_max_body_size keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseErrorPages(std::istringstream& iss) {
		std::string code;
		std::string tmpStr;
		std::vector<int> codeList;
		int codeValue;
		if (!(iss >> code)) {
			std::cerr << "Missing information after error_page keyword" << std::endl;
			return false;
		}
		if (!parseErrorCode(code, codeList))
			return false;
		if (!(iss >> tmpStr)) {
			std::cerr << "Missing path after error_page code" << std::endl;
			return false;
		}
		code = tmpStr;
		while (iss >> tmpStr) {
			if (!parseErrorCode(code, codeList))
				return false;
			code = tmpStr;
		}
		for (std::vector<int>::iterator it = codeList.begin(); it != codeList.end(); it++) {
			codeValue = *it;
			if (_errorPages.find(codeValue) == _errorPages.end()) {
				_errorPages[codeValue] = code; // we replace the value only if the key does not exist, else it is the first defined error page that is taken into account
			}
		}
		return true;
	}

	bool parseErrorCode(std::string &code, std::vector<int> &codeList) {
		int codeValue;
		size_t idx = code.find_first_not_of("0123456789");
		if (idx != std::string::npos) {
			if (code[idx] == '-') {
				code[idx] = ' ';
				std::istringstream range(code);
				int start, end;
				std::string check;
				if (!(range >> start >> end)) {
					std::cerr << "Invalid format for code range in error_page" << std::endl;
					return false;
				}
				if (range >> check) {
					std::cerr << "Too many arguments for code range in error_page" << std::endl;
					return false;
				}
				if (start < 100 || start > 599 || end < 100 || end > 599) {
					std::cerr << "Invalid error code in range: " << start << "-" << end << std::endl;
					return false;
				}
				for (int i = start; i <= end; i++)
					codeList.push_back(i);
				return true;
			}
			else {
				std::cerr << "Invalid character in error code: " << code << std::endl;
				return false;
			}
		}
		codeValue = std::strtol(code.c_str(), NULL, 10);
		if (codeValue < 100 || codeValue > 599) {
			std::cerr << "Invalid error code: " << codeValue << std::endl;
			return false;
		}
		codeList.push_back(codeValue);
		return true;
	}

	bool parseIndex(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << "Missing information after index keyword" << std::endl;
			return false;
		}
		_indexPages.push_back(value);
		while (iss >> value)
			_indexPages.push_back(value);
		return true;
	}
};
