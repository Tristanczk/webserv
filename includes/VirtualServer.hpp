#pragma once

#include "webserv.hpp"
#include <algorithm>
#include <stdexcept>

class VirtualServer {
public:
	VirtualServer() {
		std::memset(&_address, 0, sizeof(_address));
		_address.sin_family = AF_INET;
		_address.sin_port = htons(DEFAULT_PORT);
		_address.sin_addr.s_addr = htonl(INADDR_ANY);
		_rootDir = "./www/";
		_autoIndex = false;
		_bodySize = DEFAULT_BODY_SIZE;
		_return.first = -1;
		initKeywordMap();
	}

	~VirtualServer(){};

	bool init(std::istream& config) {
		std::string line, keyword, val;
		bool empty = true;
		while (std::getline(config, line)) {
			std::istringstream iss(line);
			if (!(iss >> keyword) || keyword[0] == '#') {
				continue;
			} else if (keyword == "}") {
				return empty ? configFileError("empty server block") : true;
			} else if (keyword == "location") {
				Location location(_rootDir, _autoIndex, _indexPages, _return);
				if (!location.initUri(iss)) {
					return false;
				}
				if (!location.parseLocationContent(config)) {
					return false;
				}
				_locations.push_back(location);
			} else {
				try {
					KeywordHandler handler = _keywordHandlers.at(keyword);
					if (!(this->*handler)(iss)) {
						return false;
					}
				} catch (const std::out_of_range& e) {
					return configFileError("invalid keyword in configuration file: " + keyword);
				}
			}
			empty = false;
		}
		return configFileError("missing closing bracket for server");
	}

	VirtualServerMatch isMatching(in_port_t port, in_addr_t addr, std::string serverName) const {
		const bool addrIsAny =
			addr == htonl(INADDR_ANY) || _address.sin_addr.s_addr == htonl(INADDR_ANY);
		if (port != _address.sin_port || (!addrIsAny && addr != _address.sin_addr.s_addr)) {
			return VS_MATCH_NONE;
		}
		if (serverName.empty() ||
			std::find(_serverNames.begin(), _serverNames.end(), serverName) == _serverNames.end()) {
			return addrIsAny ? VS_MATCH_INADDR_ANY : VS_MATCH_IP;
		}
		return addrIsAny ? VS_MATCH_SERVER : VS_MATCH_BOTH;
	}

	Location* findMatchingLocation(std::string const& requestPath) {
		int curRegex = -1;
		int curPrefix = -1;
		int prefixLength = 0;
		for (size_t i = 0; i < _locations.size(); ++i) {
			int matchLevel;
			try {
				matchLevel = _locations[i].isMatching(requestPath);
			} catch (const RegexError& e) {
				std::cerr << e.what() << std::endl;
				return NULL;
			}
			if (matchLevel == LOCATION_MATCH_EXACT) {
				return &_locations[i];
			} else if (matchLevel == LOCATION_MATCH_REGEX && curRegex == -1) {
				curRegex = i;
			} else if (matchLevel > prefixLength) {
				curPrefix = i;
				prefixLength = matchLevel;
			}
		}
		if (curRegex != -1) {
			return &_locations[curRegex];
		} else if (curPrefix != -1) {
			return &_locations[curPrefix];
		} else {
			return NULL;
		}
	}

	in_port_t getPort() const { return _address.sin_port; }
	in_addr_t getAddr() const { return _address.sin_addr.s_addr; }
	std::string getRootDir() const { return _rootDir; }
	bool getAutoIndex() const { return _autoIndex; }
	size_t getBodySize() const { return _bodySize; }
	struct sockaddr_in getAddress() const { return _address; }
	std::vector<std::string> const& getServerNames() const { return _serverNames; }
	std::vector<Location> const& getLocations() const { return _locations; }
	std::map<int, std::string> const& getErrorPages() const { return _errorPages; }
	std::vector<std::string> const& getIndexPages() const { return _indexPages; }

private:
	typedef bool (VirtualServer::*KeywordHandler)(std::istringstream&);
	struct sockaddr_in _address;
	std::vector<std::string> _serverNames;
	std::string _rootDir;
	bool _autoIndex;
	size_t _bodySize;
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
		_keywordHandlers["client_max_body_size"] = &VirtualServer::parseClientMaxBodySize;
		_keywordHandlers["error_page"] = &VirtualServer::parseErrorPages;
		_keywordHandlers["index"] = &VirtualServer::parseIndex;
		_keywordHandlers["return"] = &VirtualServer::parseReturn;
	}

	bool parseListen(std::istringstream& iss) {
		std::string value, host;
		size_t port;
		if (!(iss >> value)) {
			return configFileError("missing information after listen keyword");
		}
		size_t idx = value.find(':');
		if (idx == std::string::npos) {
			if (value.find_first_not_of("0123456789") != std::string::npos) {
				if (!getIpValue(value, _address.sin_addr.s_addr)) {
					return configFileError(ERROR_ADDRESS);
				}
			} else {
				port = std::strtol(value.c_str(), NULL, 10);
				if (port > MAX_PORT) {
					return configFileError(ERROR_PORT);
				}
				_address.sin_port = htons(port);
			}
		} else {
			value[idx] = ' ';
			std::istringstream hostPort(value);
			if (!(hostPort >> host >> port)) {
				return configFileError(ERROR_LISTEN_FORMAT);
			}
			if (!getIpValue(host, _address.sin_addr.s_addr)) {
				return configFileError(ERROR_ADDRESS);
			}
			if (port > MAX_PORT) {
				return configFileError(ERROR_PORT);
			}
			_address.sin_port = htons(port);
			if (hostPort >> value) {
				return configFileError(ERROR_LISTEN_FORMAT);
			}
		}
		if (iss >> value) {
			return configFileError("too many arguments after listen keyword");
		}
		return true;
	}

	bool parseServerNames(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			return configFileError("missing information after server_name keyword");
		}
		_serverNames.push_back(value);
		while (iss >> value) {
			_serverNames.push_back(value);
		}
		return true;
	}

	bool parseAutoIndex(std::istringstream& iss) { return ::parseAutoIndex(iss, _autoIndex); }
	bool parseErrorPages(std::istringstream& iss) { return ::parseErrorPages(iss, _errorPages); }
	bool parseIndex(std::istringstream& iss) { return ::parseIndex(iss, _indexPages); }
	bool parseReturn(std::istringstream& iss) { return ::parseReturn(iss, _return); }

	bool parseRoot(std::istringstream& iss) {
		return ::parseDirectory(iss, _rootDir, "server", "root") &&
			   validateUri(_rootDir, "server root");
	}

	bool parseClientMaxBodySize(std::istringstream& iss) {
		return parseSize(iss, _bodySize, "client_max_body_size", 0, SIZE_LIMIT);
	}

	bool parseSize(std::istringstream& iss, size_t& size, const std::string& keyword,
				   size_t minLimit, size_t maxLimit) {
		std::string value;
		if (!(iss >> value)) {
			return configFileError("missing information after " + keyword);
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			return configFileError("invalid character for " + keyword);
		}
		size = std::strtol(value.c_str(), NULL, 10);
		if (size == LONG_MAX) {
			return configFileError("invalid value for " + keyword);
		}
		if (value[idx] != '\0') {
			if (value[idx + 1] != '\0') {
				return configFileError("invalid character after suffix for bytes value in " +
									   keyword + " directive");
			}
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
		if (size < minLimit || size > maxLimit) {
			return configFileError(keyword + " must be between " + toString(minLimit) + " and " +
								   toString(maxLimit));
		}
		if (iss >> value) {
			return configFileError("too many arguments after " + keyword + " keyword");
		}
		return true;
	}
};
