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
		_bodySize = BODY_SIZE;
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
		_keywordHandlers["client_buffer_size"] = &VirtualServer::parseClientBodyBufferSize;
		_keywordHandlers["client_max_body_size"] = &VirtualServer::parseClientMaxBodySize;
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
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing information after root keyword" << std::endl;
			return false;
		}
		_rootDir = value;
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR << "Too many arguments after root keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseAutoIndex(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing information after autoindex keyword"
					  << std::endl;
			return false;
		}
		if (value == "on")
			_autoIndex = true;
		else if (value == "off")
			_autoIndex = false;
		else {
			std::cerr << CONFIG_FILE_ERROR << "Invalid value for autoindex keyword: " << value
					  << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR << "Too many arguments after autoindex keyword"
					  << std::endl;
			return false;
		}
		return true;
	}

	bool parseClientBodyBufferSize(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Missing information after client_buffer_size keyword" << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid character for client_buffer_size"
					  << std::endl;
			return false;
		}
		_bufferSize = std::strtol(value.c_str(), NULL, 10);
		if (_bufferSize == LONG_MAX) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid value for client_buffer_size"
					  << std::endl;
			return false;
		}
		if (value[idx] != '\0') {
			switch (std::tolower(value[idx])) {
			case 'k':
				_bufferSize <<= 10;
				break;
			case 'm':
				_bufferSize <<= 20;
				break;
			// case 'g':
			// 	_bufferSize <<= 30;
			// 	break;
			// we do not accept gigabytes as the size would be too large
			default:
				std::cerr << CONFIG_FILE_ERROR
						  << "Invalid suffix for bytes value, valid suffix are: k, K, m, M"
						  << std::endl;
				return false;
			}
			if (value[idx + 1] != '\0') {
				std::cerr << CONFIG_FILE_ERROR << "Invalid character after suffix for bytes value"
						  << std::endl;
				return false;
			}
		}
		if (_bufferSize > BUFFER_SIZE_SERVER_LIMIT || _bufferSize < MIN_BUFFER_SIZE) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Buffer size must be between " << MIN_BUFFER_SIZE
					  << " and " << BUFFER_SIZE_SERVER_LIMIT << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Too many arguments after client_buffer_size keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseClientMaxBodySize(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Missing information after client_buffer_size keyword" << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid character for client_buffer_size"
					  << std::endl;
			return false;
		}
		_bodySize = std::strtol(value.c_str(), NULL, 10);
		if (_bodySize == LONG_MAX) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid value for client_buffer_size"
					  << std::endl;
			return false;
		}
		if (value[idx] != '\0') {
			switch (std::tolower(value[idx])) {
			case 'k':
				_bodySize <<= 10;
				break;
			case 'm':
				_bodySize <<= 20;
				break;
			case 'g':
				_bodySize <<= 30;
				break;
			default:
				std::cerr << CONFIG_FILE_ERROR
						  << "Invalid suffix for bytes value, valid suffix are: k, K, m, M, g, G"
						  << std::endl;
				return false;
			}
			if (value[idx + 1] != '\0') {
				std::cerr << CONFIG_FILE_ERROR << "Invalid character after suffix for bytes value"
						  << std::endl;
				return false;
			}
		}
		if (_bodySize > BODY_SIZE_LIMIT) {
			std::cerr << CONFIG_FILE_ERROR << "Body size too big, maximum is: " << BODY_SIZE_LIMIT
					  << " bytes" << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Too many arguments after client_max_body_size keyword" << std::endl;
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
			std::cerr << CONFIG_FILE_ERROR << "Missing information after error_page keyword"
					  << std::endl;
			return false;
		}
		if (!parseErrorCode(code, codeList))
			return false;
		if (!(iss >> tmpStr)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing path after error_page code" << std::endl;
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
				_errorPages[codeValue] =
					code; // we replace the value only if the key does not exist, else it is the
						  // first defined error page that is taken into account
			}
		}
		return true;
	}

	bool parseErrorCode(std::string& code, std::vector<int>& codeList) {
		int codeValue;
		size_t idx = code.find_first_not_of("0123456789");
		if (idx != std::string::npos) {
			if (code[idx] == '-') {
				code[idx] = ' ';
				std::istringstream range(code);
				int start, end;
				std::string check;
				if (!(range >> start >> end)) {
					std::cerr << CONFIG_FILE_ERROR << "Invalid format for code range in error_page"
							  << std::endl;
					return false;
				}
				if (range >> check) {
					std::cerr << CONFIG_FILE_ERROR
							  << "Too many arguments for code range in error_page" << std::endl;
					return false;
				}
				if (start < 100 || start > 599 || end < 100 || end > 599) {
					std::cerr << CONFIG_FILE_ERROR << "Invalid error code in range: " << start
							  << "-" << end << std::endl;
					return false;
				}
				for (int i = start; i <= end; i++)
					codeList.push_back(i);
				return true;
			} else {
				std::cerr << CONFIG_FILE_ERROR << "Invalid character in error code: " << code
						  << std::endl;
				return false;
			}
		}
		codeValue = std::strtol(code.c_str(), NULL, 10);
		if (codeValue < 100 || codeValue > 599) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid error code: " << codeValue << std::endl;
			return false;
		}
		codeList.push_back(codeValue);
		return true;
	}

	bool parseIndex(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing information after index keyword"
					  << std::endl;
			return false;
		}
		_indexPages.push_back(value);
		while (iss >> value)
			_indexPages.push_back(value);
		return true;
	}

	bool parseReturn(std::istringstream& iss) {
		std::string value;
		if (_return.first != -1) {
			std::cerr << CONFIG_FILE_ERROR << "Multiple return instructions" << std::endl;
			return false;
		}
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing information after return keyword"
					  << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx != std::string::npos) {
			_return.first = 302;
			_return.second = value;
		}
		else {
			_return.first = std::strtol(value.c_str(), NULL, 10);
			if (_return.first == LONG_MAX) {
				std::cerr << CONFIG_FILE_ERROR << "Invalid value for return code" << std::endl;
				return false;
			}
			if (!(_return.first >= 0 && _return.first <= 599)) {
				std::cerr << CONFIG_FILE_ERROR << "Invalid return code: " << _return.first
						  << std::endl;
				return false;
			}
			if (!(iss >> value)) {
				std::cerr << CONFIG_FILE_ERROR << "Missing redirection url or text after return code" << std::endl;
				return false;
			}
			_return.second = value;
		}
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR << "Too many arguments after return keyword"
					  << std::endl;
			return false;
		}
		return true;
	}
};
