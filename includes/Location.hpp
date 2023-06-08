#pragma once

#include "webserv.hpp"

class Location {
public:
	Location(const std::string& rootDir, bool autoIndex, size_t bufferSize, size_t bodySize,
			 const std::map<int, std::string>& serverErrorPages,
			 const std::vector<std::string>& serverIndexPages)
		: _modifier(NONE), _uri(""), _rootDir(rootDir), _autoIndex(autoIndex),
		  _bufferSize(bufferSize), _bodySize(bodySize), _serverErrorPages(serverErrorPages),
		  _serverIndexPages(serverIndexPages) {
		initKeywordMap();
	}

	~Location(){};

	bool initUri(std::istringstream& iss) {
		std::string modifier, uri, check;
		if (!(iss >> modifier >> uri)) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Wrong syntax for location, syntax must be 'location [modifier] uri {'"
					  << std::endl;
			return false;
		}
		if (uri == "{") {
			_uri = modifier;
			_modifier = NONE;
		} else {
			_uri = uri;
			if (modifier == "~")
				_modifier = REGEX;
			else if (modifier == "=")
				_modifier = EXACT;
			else {
				std::cerr << CONFIG_FILE_ERROR << "Invalid modifier for location: " << modifier
						  << std::endl;
				return false;
			}
			if (!(iss >> check)) {
				std::cerr << CONFIG_FILE_ERROR
						  << "Wrong syntax for location, syntax must be 'location [modifier] uri {'"
						  << std::endl;
				return false;
			}
		}
		if (iss >> check) {
			std::cerr << CONFIG_FILE_ERROR << "Too many arguments for location" << std::endl;
			return false;
		}
		return true;
	}

	bool parseLocationContent(std::istream& config) {
		std::string line, keyword, val;
		for (; std::getline(config, line);) {
			std::istringstream iss(line);
			if (!(iss >> keyword))
				continue;
			if (keyword == "}") {
				checkIndexPages();
				checkErrorPages();
				return true;
			} else if (keyword[0] == '#')
				continue;
			else {
				try {
					KeywordHandler handler = _keywordHandlers[keyword];
					if (!(this->*handler)(iss))
						return false;
				} catch (const std::exception& e) {
					std::cerr << CONFIG_FILE_ERROR
							  << "Invalid keyword in location block: " << keyword << std::endl;
					return false;
				}
			}
		}
		std::cerr << CONFIG_FILE_ERROR << "Missing closing bracket for location block" << std::endl;
		return false;
	};

	// the int returned depend on the matching
	// -2 = exact match
	// -1 = regex match
	// 0 = no match
	// any positive number is a prefix match, the int returned is the length of the matching prefix
	int isMatching(std::string const & requestPath) const {
		if (_modifier == EXACT)
		{
			if (requestPath == _uri)
				return -2;
			else
				return 0;
		}
		else if (_modifier == REGEX)
		{
			regex_t reg;
			int		regint;
			
			if (regcomp(&reg, _uri.c_str(), REG_EXTENDED) != 0)
				throw(RegexError());
			else
			{
				regint = regexec(&reg, requestPath.c_str(), 0, NULL, 0);
				regfree(&reg);
				if (regint == 0)
					return -1;
				else if (regint == REG_NOMATCH)
					return 0;
				else
					throw(RegexError());
			}
		}
		else
			return comparePrefix(_uri, requestPath);
		return 0;
	}

	// TODO : delete this function as it uses inet_ntoa which is not allowed for the project
	void printLocationInformation() const {
		std::cout << "Location information:" << std::endl;
		std::cout << "Modifier: "
				  << (_modifier == NONE ? "none" : (_modifier == REGEX ? "regex" : "exact"))
				  << std::endl;
		std::cout << "URI: " << _uri << std::endl;
		std::cout << "Root directory: " << _rootDir << std::endl;
		std::cout << "Autoindex: " << (_autoIndex ? "on" : "off") << std::endl;
		std::cout << "Client body buffer size: " << _bufferSize << std::endl;
		std::cout << "Client max body size: " << _bodySize << std::endl;
		std::cout << "Error pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = _errorPages.begin();
			 it != _errorPages.end(); it++)
			std::cout << "error " << it->first << ": " << it->second << std::endl;
		std::cout << "Index pages: ";
		for (std::vector<std::string>::const_iterator it = _indexPages.begin();
			 it != _indexPages.end(); it++)
			std::cout << *it << ", ";
		std::cout << std::endl;
	}

private:
	typedef bool (Location::*KeywordHandler)(std::istringstream&);
	typedef enum e_modifier { NONE, REGEX, EXACT } t_modifier;
	t_modifier _modifier;
	std::string _uri;
	std::string _rootDir;
	bool _autoIndex;
	std::size_t _bufferSize;
	std::size_t _bodySize;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	const std::map<int, std::string>& _serverErrorPages;
	const std::vector<std::string>& _serverIndexPages;
	std::map<std::string, KeywordHandler> _keywordHandlers;

	void initKeywordMap() {
		_keywordHandlers["root"] = &Location::parseRoot;
		_keywordHandlers["autoindex"] = &Location::parseAutoIndex;
		_keywordHandlers["client_body_buffer_size"] = &Location::parseClientBodyBufferSize;
		_keywordHandlers["client_max_body_size"] = &Location::parseClientMaxBodySize;
		_keywordHandlers["error_page"] = &Location::parseErrorPages;
		_keywordHandlers["index"] = &Location::parseIndex;
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
					  << "Missing information after client_body_buffer_size keyword" << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid character for client_body_buffer_size"
					  << std::endl;
			return false;
		}
		_bufferSize = std::strtol(value.c_str(), NULL, 10);
		if (_bufferSize == LONG_MAX) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid value for client_body_buffer_size"
					  << std::endl;
			return false;
		}
		if (value[idx] != '\0') {
			switch (std::tolower(value[idx])) {
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
		if (_bufferSize > BUFFER_SIZE_SERVER_LIMIT) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Buffer size too big, maximum is: " << BUFFER_SIZE_SERVER_LIMIT << " bytes"
					  << std::endl;
			return false;
		}
		if (iss >> value) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Too many arguments after client_body_buffer_size keyword" << std::endl;
			return false;
		}
		return true;
	}

	bool parseClientMaxBodySize(std::istringstream& iss) {
		std::string value;
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR
					  << "Missing information after client_body_buffer_size keyword" << std::endl;
			return false;
		}
		size_t idx = value.find_first_not_of("0123456789");
		if (idx == 0) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid character for client_body_buffer_size"
					  << std::endl;
			return false;
		}
		_bodySize = std::strtol(value.c_str(), NULL, 10);
		if (_bodySize == LONG_MAX) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid value for client_body_buffer_size"
					  << std::endl;
			return false;
		}
		if (value[idx] != '\0') {
			switch (std::tolower(value[idx])) {
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

	void checkIndexPages() {
		if (_indexPages.empty()) {
			for (std::vector<std::string>::const_iterator it = _serverIndexPages.begin();
				 it != _serverIndexPages.end(); it++)
				_indexPages.push_back(*it);
		}
	}

	void checkErrorPages() {
		// Shouldn't we also check _errorPages.empty() like in checkIndexPages?
		for (std::map<int, std::string>::const_iterator it = _serverErrorPages.begin();
			 it != _serverErrorPages.end(); it++) {
			if (_errorPages.find(it->first) == _errorPages.end())
				_errorPages[it->first] = it->second;
		}
	}
};
