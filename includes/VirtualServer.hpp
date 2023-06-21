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

	bool init(std::istream& config) {
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
					return configFileError("invalid keyword in configuration file: " + keyword);
				}
			}
		}
		return configFileError("missing closing bracket for server");
	}

	t_vsmatch isMatching(in_port_t port, in_addr_t addr, std::string serverName) const {
		const bool addrIsAny = addr == htonl(INADDR_ANY);
		if (port != _address.sin_port)
			return VS_MATCH_NONE;
		if (!addrIsAny && addr != _address.sin_addr.s_addr)
			return VS_MATCH_NONE;
		if (serverName.empty())
			return addrIsAny ? VS_MATCH_INADDR_ANY : VS_MATCH_IP;
		// TODO _serverNames.find
		for (std::vector<std::string>::const_iterator it = _serverNames.begin();
			 it != _serverNames.end(); it++) {
			if (*it == serverName)
				return addrIsAny ? VS_MATCH_SERVER : VS_MATCH_BOTH;
		}
		return addrIsAny ? VS_MATCH_INADDR_ANY : VS_MATCH_IP;
	}

	Location* findMatchingLocation(std::string const& requestPath) {
		int curRegex = -1;
		int curPrefix = -1;
		int prefixLength = 0;
		for (size_t i = 0; i < _locations.size(); ++i) {
			int matchLevel;
			try {
				matchLevel = _locations[i].isMatching(requestPath);
			} catch (const std::exception& e) { // ???
				std::cerr << e.what() << std::endl;
				return NULL;
			}
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
	struct sockaddr_in getAddress() const { return _address; }
	const std::vector<std::string>& getServerNames() const { return _serverNames; }
	std::size_t getBufferSize() const { return _bufferSize; }

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
		if (!(iss >> value))
			return configFileError("missing information after listen keyword");
		size_t idx = value.find(':');
		if (idx == std::string::npos) {
			if (value.find_first_not_of("0123456789") != std::string::npos) {
				if (!getIpValue(value, _address.sin_addr.s_addr))
					return configFileError(ERROR_ADDRESS);
			} else {
				port = std::strtol(value.c_str(), NULL, 10);
				if (port > MAX_PORT || port < 0)
					return configFileError(ERROR_PORT);
				_address.sin_port = htons(port);
			}
		} else {
			value[idx] = ' ';
			std::istringstream hostPort(value);
			if (!(hostPort >> host >> port))
				return configFileError(ERROR_LISTEN_FORMAT);
			if (!getIpValue(host, _address.sin_addr.s_addr))
				return configFileError(ERROR_ADDRESS);
			if (port > MAX_PORT)
				return configFileError(ERROR_PORT);
			_address.sin_port = htons(port);
			if (hostPort >> value)
				return configFileError(ERROR_LISTEN_FORMAT);
		}
		if (iss >> value)
			return configFileError("too many arguments after listen keyword");
		return true;
	}

	bool parseServerNames(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value))
			return configFileError("missing information after server_name keyword");
		_serverNames.push_back(value);
		while (iss >> value)
			_serverNames.push_back(value);
		return true;
	}

	bool parseAutoIndex(std::istringstream& iss) { return ::parseAutoIndex(iss, _autoIndex); }
	bool parseErrorPages(std::istringstream& iss) { return ::parseErrorPages(iss, _errorPages); }
	bool parseIndex(std::istringstream& iss) { return ::parseIndex(iss, _indexPages); }
	bool parseReturn(std::istringstream& iss) { return ::parseReturn(iss, _return); }
	bool parseRoot(std::istringstream& iss) { return ::parseRoot(iss, _rootDir); }

	bool parseClientBufferSize(std::istringstream& iss) {
		return parseSize(iss, _bufferSize, "client_buffer_size", MIN_BUFFER_SIZE,
						 BUFFER_SIZE_SERVER_LIMIT);
	}

	bool parseClientMaxBodySize(std::istringstream& iss) {
		return parseSize(iss, _bodySize, "client_max_body_size", 0, SIZE_LIMIT);
	}

	bool parseClientMaxHeaderSize(std::istringstream& iss) {
		return parseSize(iss, _headerSize, "client_max_header_size", 0, SIZE_LIMIT);
	}

	bool parseSize(std::istringstream& iss, std::size_t& size, const std::string& keyword,
				   std::size_t minLimit, std::size_t maxLimit) {
		std::string value;
		if (!(iss >> value))
			return configFileError("missing information after " + keyword);
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0)
			return configFileError("invalid character for " + keyword);
		size = std::strtol(value.c_str(), NULL, 10);
		if (size == LONG_MAX || size < 0)
			return configFileError("invalid value for " + keyword);
		if (value[idx] != '\0') {
			if (value[idx + 1] != '\0')
				return configFileError("invalid character after suffix for bytes value in " +
									   keyword + " directive");
			switch (std::tolower(value[idx])) {
			case 'k':
				size <<= 10;
				break;
			case 'm':
				size <<= 20;
				break;
			default:
				return configFileError("invalid suffix for bytes value in " + keyword +
									   " directive, valid suffix are: k, K, m, M");
			}
		}
		if (size < minLimit || size > maxLimit)
			return configFileError(keyword + " must be between " + toString(minLimit) + " and " +
								   toString(maxLimit));
		if (iss >> value)
			return configFileError("too many arguments after " + keyword + " keyword");
		return true;
	}
};
