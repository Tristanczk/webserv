#pragma once

#include "webserv.hpp"

class Location {
public:
	Location(const std::string& rootDir, bool autoIndex, const std::map<int, std::string>& serverErrorPages,
			 const std::vector<std::string>& serverIndexPages, const std::pair<long, std::string>& serverReturn)
		: _modifier(NONE), _uri(""), _rootDir(rootDir), _autoIndex(autoIndex),
		 	_return(-1, ""), _serverErrorPages(serverErrorPages),
		  _serverIndexPages(serverIndexPages), _serverReturn(serverReturn) {
		initKeywordMap();
		for (int i = 0; i < NO_METHOD; i++)
			_allowedMethods[i] = true;
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
				checkReturn();
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

	int isMatching(const std::string& requestPath) const {
		switch (_modifier) {
		case NONE:
			return comparePrefix(_uri, requestPath);
		case REGEX:
			return doesRegexMatch(_uri.c_str(), requestPath.c_str()) ? LOCATION_MATCH_REGEX
																	 : LOCATION_MATCH_NONE;
		case EXACT:
			return requestPath == _uri ? LOCATION_MATCH_EXACT : LOCATION_MATCH_NONE;
		}
		return LOCATION_MATCH_NONE;
	}

	void printLocationInformation() const {
		std::cout << "Location information:" << std::endl;
		std::cout << "Modifier: "
				  << (_modifier == NONE ? "none" : (_modifier == REGEX ? "regex" : "exact"))
				  << std::endl;
		std::cout << "URI: " << _uri << std::endl;
		std::cout << "Root directory: " << _rootDir << std::endl;
		std::cout << "Autoindex: " << (_autoIndex ? "on" : "off") << std::endl;
		std::cout << "Return code: " << _return.first << ", url: " << _return.second << std::endl;
		std::cout << "Allowed methods: " << (_allowedMethods[GET] ? "GET " : "")
				  << (_allowedMethods[POST] ? "POST " : "")
				  << (_allowedMethods[DELETE] ? "DELETE " : "") << std::endl;
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
	std::pair<long, std::string> _return;
	bool _allowedMethods[NO_METHOD]; //NO_METHOD is equivalent to the number of methods in the RequestMethod enum
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	const std::map<int, std::string>& _serverErrorPages;
	const std::vector<std::string>& _serverIndexPages;
	const std::pair<long, std::string>& _serverReturn;
	std::map<std::string, KeywordHandler> _keywordHandlers;

	void initKeywordMap() {
		_keywordHandlers["root"] = &Location::parseRoot;
		_keywordHandlers["autoindex"] = &Location::parseAutoIndex;
		_keywordHandlers["error_page"] = &Location::parseErrorPages;
		_keywordHandlers["index"] = &Location::parseIndex;
		_keywordHandlers["return"] = &Location::parseReturn;
		_keywordHandlers["limit_except"] = &Location::parseMethods;
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

	bool parseMethods(std::istringstream& iss) {
		std::string method;
		if (!(iss >> method)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing information after limit_except keyword"
					  << std::endl;
			return false;
		}
		for (int i = 0; i < NO_METHOD; i++)
			_allowedMethods[i] = false;
		if (!updateMethod(method))
			return false;
		while (iss >> method) {
			if (!updateMethod(method))
				return false;
		}
		return true;
	}

	bool updateMethod(std::string const & method) {
		if (method == "GET")
		{
			if (_allowedMethods[GET])
			{
				std::cerr << CONFIG_FILE_ERROR << "Multiple GET instructions in limit_except directive" << std::endl;
				return false;
			}
			_allowedMethods[GET] = true;
		}
		else if (method == "POST")
		{
			if (_allowedMethods[POST])
			{
				std::cerr << CONFIG_FILE_ERROR << "Multiple POST instructions in limit_except directive" << std::endl;
				return false;
			}
			_allowedMethods[POST] = true;
		}
		else if (method == "DELETE")
		{
			if (_allowedMethods[DELETE])
			{
				std::cerr << CONFIG_FILE_ERROR << "Multiple DELETE instructions in limit_except directive" << std::endl;
				return false;
			}
			_allowedMethods[DELETE] = true;
		}
		else {
			std::cerr << CONFIG_FILE_ERROR << "Invalid method in limit_except directive: " << method << std::endl;
			return false;
		}
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
		for (std::map<int, std::string>::const_iterator it = _serverErrorPages.begin();
			 it != _serverErrorPages.end(); it++) {
			if (_errorPages.find(it->first) == _errorPages.end())
				_errorPages[it->first] = it->second;
		}
	}

	void checkReturn() {
		if (_return.first == -1 && _serverReturn.first != -1)
			_return = _serverReturn;
	}
};
