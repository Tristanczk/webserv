#pragma once

#include "webserv.hpp"

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
		_bodySize = DEFAULT_SIZE;
		_headerSize = DEFAULT_SIZE;
		_errorPages[DEFAULT_ERROR] = "./www/default_error.html";
		_return.first = -1;
		initKeywordMap();
	}

	~VirtualServer(){};

	bool initServer(std::istream& config) {
		std::string line, keyword, val;
		while (std::getline(config, line)) {
			std::istringstream iss(line);
			if (!(iss >> keyword))
				continue;
			if (keyword == "}")
				return true;
			else if (keyword[0] == '#')
				continue;
			else if (keyword == "location") {
				Location location(_rootDir, _autoIndex, _errorPages, _indexPages, _return);
				if (!location.initUri(iss))
					return false;
				if (!location.parseLocationContent(config))
					return false;
				_locations.push_back(location);
			} else {
				try {
					KeywordHandler handler = _keywordHandlers.at(keyword);
					if (!(this->*handler)(iss))
						return false;
				} catch (const std::exception& e) {
					std::cerr << CONFIG_FILE_ERROR
							  << "Invalid keyword in configuration file: " << keyword << std::endl;
					return false;
				}
			}
		}
		std::cerr << CONFIG_FILE_ERROR << "Missing closing bracket for server" << std::endl;
		return false;
	}

	t_vsmatch isMatching(in_port_t port, in_addr_t addr, std::string serverName) const {
		if (port != _address.sin_port)
			return VS_MATCH_NONE;
		if (addr != htonl(INADDR_ANY) && addr != _address.sin_addr.s_addr)
			return VS_MATCH_NONE;
		if (serverName.empty()) {
			if (addr == htonl(INADDR_ANY))
				return VS_MATCH_INADDR_ANY;
			else
				return VS_MATCH_IP;
		}
		for (std::vector<std::string>::const_iterator it = _serverNames.begin(); it != _serverNames.end(); it++) 
		{
			if (*it == serverName) {
				if (addr == htonl(INADDR_ANY))
					return VS_MATCH_SERVER;
				else
					return VS_MATCH_BOTH;
			}
		}
		if (addr != htonl(INADDR_ANY))
			return VS_MATCH_IP;
		else
			return VS_MATCH_INADDR_ANY;
	}

	Location* findMatchingLocation(std::string const& requestPath) {
		int curRegex = -1;
		int curPrefix = -1;
		int prefixLength = 0;
		for (size_t i = 0; i < _locations.size(); ++i) {
			int matchLevel = _locations[i].isMatching(requestPath);
			if (matchLevel == LOCATION_MATCH_EXACT)
				return &_locations[i];
			else if (matchLevel == LOCATION_MATCH_REGEX && curRegex == -1)
				curRegex = i;
			else if (matchLevel > prefixLength) {
				curPrefix = i;
				prefixLength = matchLevel;
			}
		}
		if (curRegex != -1)
			return &_locations[curRegex];
		else if (curPrefix != -1)
			return &_locations[curPrefix];
		else
			return NULL;
	}

	in_port_t getPort() const { return _address.sin_port; }
	in_addr_t getAddr() const { return _address.sin_addr.s_addr; }
	std::vector<std::string> getServerNames() const { return _serverNames; }

	void printServerInformation() const {
		std::cout << "Server information:" << std::endl;
		std::cout << "Address: " << getIpString(_address.sin_addr.s_addr) << std::endl;
		std::cout << "Port: " << ntohs(_address.sin_port) << std::endl;
		std::cout << "Server names: ";
		for (std::vector<std::string>::const_iterator it = _serverNames.begin();
			 it != _serverNames.end(); it++)
			std::cout << *it << ", ";
		std::cout << std::endl;
		std::cout << "Root directory: " << _rootDir << std::endl;
		std::cout << "Autoindex: " << (_autoIndex ? "on" : "off") << std::endl;
		std::cout << "Client buffer size: " << _bufferSize << std::endl;
		std::cout << "Client max body size: " << _bodySize << std::endl;
		std::cout << "Client max header size: " << _headerSize << std::endl;
		std::cout << "Return code: " << _return.first << ", url: " << _return.second << std::endl;
		std::cout << "Error pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = _errorPages.begin();
			 it != _errorPages.end(); it++)
			std::cout << "error " << it->first << ": " << it->second << std::endl;
		std::cout << "Index pages: ";
		for (std::vector<std::string>::const_iterator it = _indexPages.begin();
			 it != _indexPages.end(); it++)
			std::cout << *it << ", ";
		std::cout << std::endl;
		std::cout << "Locations:" << std::endl;
		for (std::vector<Location>::const_iterator it = _locations.begin(); it != _locations.end();
			 it++)
			it->printLocationInformation();
	}

private:
	typedef bool (VirtualServer::*KeywordHandler)(std::istringstream&);
	struct sockaddr_in _address;
	std::vector<std::string> _serverNames;
	std::string _rootDir;
	bool _autoIndex;
	std::size_t _bufferSize;
	std::size_t _bodySize;
	std::size_t _headerSize;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::pair<long, std::string> _return;
	std::vector<Location> _locations;
	std::map<std::string, KeywordHandler> _keywordHandlers;

	void initKeywordMap() {
		_keywordHandlers["listen"] = &VirtualServer::parseListen;
		_keywordHandlers["server_name"] = &VirtualServer::parseServerNames;
		_keywordHandlers["root"] = &VirtualServer::parseRoot;
		_keywordHandlers["autoindex"] = &VirtualServer::parseAutoIndex;
		_keywordHandlers["client_buffer_size"] = &VirtualServer::parseClientBufferSize;
		_keywordHandlers["client_max_body_size"] = &VirtualServer::parseClientMaxBodySize;
		_keywordHandlers["client_max_header_size"] = &VirtualServer::parseClientMaxHeaderSize;
		_keywordHandlers["error_page"] = &VirtualServer::parseErrorPages;
		_keywordHandlers["index"] = &VirtualServer::parseIndex;
		_keywordHandlers["return"] = &VirtualServer::parseReturn;
	}

	bool parseListen(std::istringstream& iss) {
		std::string value, host;
		size_t port;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing information after listen keyword"
					  << std::endl;
			return false;
		}
		size_t idx = value.find(':');
		if (idx == std::string::npos) {
			if (value.find_first_not_of("0123456789") != std::string::npos) {
				if (!getIpValue(value, _address.sin_addr.s_addr)) {
					std::cerr << CONFIG_FILE_ERROR
							  << "Invalid IPv4 address format in listen instruction" << std::endl;
					return false;
				}
			} else {
				port = std::strtol(value.c_str(), NULL, 10);
				if (port > 65535) {
					std::cerr << CONFIG_FILE_ERROR << "Invalid port number in listen instruction"
							  << std::endl;
					return false;
				}
				_address.sin_port = htons(port);
			}
		} else {
			value[idx] = ' ';
			std::istringstream hostPort(value);
			if (!(hostPort >> host >> port)) {
				std::cerr << CONFIG_FILE_ERROR
						  << "Invalid format for host:port in listen instruction" << std::endl;
				return false;
			}
			if (!getIpValue(host, _address.sin_addr.s_addr)) {
				std::cerr << CONFIG_FILE_ERROR
						  << "Invalid IPv4 address format in listen instruction" << std::endl;
				return false;
			}
			if (port > 65535) {
				std::cerr << CONFIG_FILE_ERROR << "Invalid port number in listen instruction"
						  << std::endl;
				return false;
			}
			_address.sin_port = htons(port);
			if (hostPort >> value) {
				std::cerr << CONFIG_FILE_ERROR
						  << "Invalid format for host:port in listen instruction" << std::endl;
				return false;
			}
		}
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR << "Too many arguments after listen keyword"
					  << std::endl;
			return false;
		}
		return true;
	}

	bool parseServerNames(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing information after server_name keyword"
					  << std::endl;
			return false;
		}
		_serverNames.push_back(value);
		while (iss >> value)
			_serverNames.push_back(value);
		return true;
	}

	bool parseRoot(std::istringstream& iss) {
		if (!::parseRoot(iss, _rootDir))
			return false;
		return true;
	}

	bool parseAutoIndex(std::istringstream& iss) {
		if (!::parseAutoIndex(iss, _autoIndex))
			return false;
		return true;
	}

	bool parseClientBufferSize(std::istringstream& iss) {
		std::string keyword = "client_buffer_size";
		if (!parseSize(iss, _bufferSize, keyword, MIN_BUFFER_SIZE, BUFFER_SIZE_SERVER_LIMIT))
			return false;
		return true;
	}

	bool parseClientMaxBodySize(std::istringstream& iss) {
		std::string keyword = "client_max_body_size";
		if (!parseSize(iss, _bodySize, keyword, 0, SIZE_LIMIT))
			return false;
		return true;
	}

	bool parseClientMaxHeaderSize(std::istringstream& iss) {
		std::string keyword = "client_max_header_size";
		if (!parseSize(iss, _headerSize, keyword, 0, SIZE_LIMIT))
			return false;
		return true;
	}

	bool parseSize(std::istringstream& iss, std::size_t& size, std::string& keyword, std::size_t minLimit, std::size_t maxLimit) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Missing information after " << keyword << "keyword" << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid character for " << keyword
					  << std::endl;
			return false;
		}
		size = std::strtol(value.c_str(), NULL, 10);
		if (size == LONG_MAX) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid value for " << keyword
					  << std::endl;
			return false;
		}
		if (value[idx] != '\0') {
			switch (std::tolower(value[idx])) {
			case 'k':
				size <<= 10;
				break;
			case 'm':
				size <<= 20;
				break;
			default:
				std::cerr << CONFIG_FILE_ERROR
						  << "Invalid suffix for bytes value in " << keyword << " directive, valid suffix are: k, K, m, M"
						  << std::endl;
				return false;
			}
			if (value[idx + 1] != '\0') {
				std::cerr << CONFIG_FILE_ERROR << "Invalid character after suffix for bytes value in " << keyword << " directive"
						  << std::endl;
				return false;
			}
		}
		if (size < minLimit || size > maxLimit) {
			std::cerr << CONFIG_FILE_ERROR
					  << keyword << " must be between " << minLimit
					  << " and " << maxLimit << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Too many arguments after " << keyword << " keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseErrorPages(std::istringstream& iss) {
		if (!::parseErrorPages(iss, _errorPages))
			return false;
		return true;
	}

	bool parseIndex(std::istringstream& iss) {
		if (!::parseIndex(iss, _indexPages))
			return false;
		return true;
	}

	bool parseReturn(std::istringstream& iss) {
		if (!::parseReturn(iss, _return))
			return false;
		return true;
	}
};
