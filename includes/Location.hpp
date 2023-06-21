#pragma once

#include "webserv.hpp"

class Location {
public:
	Location(const std::string& rootDir, bool autoIndex,
			 const std::map<int, std::string>& serverErrorPages,
			 const std::vector<std::string>& serverIndexPages,
			 const std::pair<long, std::string>& serverReturn)
		: _modifier(NONE), _uri(""), _rootDir(rootDir), _autoIndex(autoIndex), _return(-1, ""),
		  _serverErrorPages(serverErrorPages), _serverIndexPages(serverIndexPages),
		  _serverReturn(serverReturn) {
		initKeywordMap();
		for (int i = 0; i < NO_METHOD; i++)
			_allowedMethods[i] = true;
	}

	~Location(){};

	bool initUri(std::istringstream& iss) {
		std::string modifier, uri, check;
		if (!(iss >> modifier >> uri))
			return configFileError(ERROR_LOCATION);
		if (uri == "{") {
			_uri = modifier;
			_modifier = NONE;
		} else {
			_uri = uri;
			if (modifier == "~")
				_modifier = REGEX;
			else if (modifier == "=")
				_modifier = EXACT;
			else
				return configFileError("invalid modifier for location: " + modifier);
			if (!(iss >> check))
				return configFileError(ERROR_LOCATION);
		}
		if (iss >> check)
			return configFileError("too many arguments for location");
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
					return configFileError("invalid keyword in location block: " + keyword);
				}
			}
		}
		return configFileError("missing closing bracket for location block");
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
	bool _allowedMethods[NO_METHOD]; // NO_METHOD is equivalent to the number of methods in the
									 // RequestMethod enum
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

	bool parseRoot(std::istringstream& iss) { return ::parseRoot(iss, _rootDir); }
	bool parseAutoIndex(std::istringstream& iss) { return ::parseAutoIndex(iss, _autoIndex); }
	bool parseErrorPages(std::istringstream& iss) { return ::parseErrorPages(iss, _errorPages); }
	bool parseIndex(std::istringstream& iss) { return ::parseIndex(iss, _indexPages); }
	bool parseReturn(std::istringstream& iss) { return ::parseReturn(iss, _return); }

	bool parseMethods(std::istringstream& iss) {
		std::string method;
		if (!(iss >> method))
			return configFileError("missing information after limit_except keyword");
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

	bool updateMethod(std::string const& method) {
		if (method == "GET") {
			if (_allowedMethods[GET])
				return configFileError("multiple GET instructions in limit_except directive");
			_allowedMethods[GET] = true;
		} else if (method == "POST") {
			if (_allowedMethods[POST])
				return configFileError("multiple POST instructions in limit_except directive");
			_allowedMethods[POST] = true;
		} else if (method == "DELETE") {
			if (_allowedMethods[DELETE])
				return configFileError("multiple DELETE instructions in limit_except directive");
			_allowedMethods[DELETE] = true;
		} else
			return configFileError("invalid method in limit_except directive: " + method);
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
