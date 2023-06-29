#pragma once

#include "webserv.hpp"

class Location {
public:
	Location(const std::string& rootDir, bool autoIndex,
			 const std::map<int, std::string>& serverErrorPages,
			 const std::vector<std::string>& serverIndexPages,
			 const std::pair<long, std::string>& serverReturn)
		: _modifier(NONE), _uri(""), _rootDir(rootDir), _cgiExec(""), _autoIndex(autoIndex),
		  _return(-1, ""), _serverErrorPages(serverErrorPages), _serverIndexPages(serverIndexPages),
		  _serverReturn(serverReturn) {
		initKeywordMap();
		std::fill_n(_allowedMethods, NO_METHOD, true);
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
		bool empty = true;
		for (; std::getline(config, line);) {
			std::istringstream iss(line);
			if (!(iss >> keyword) || keyword[0] == '#')
				continue;
			else if (keyword == "}") {
				if (empty)
					return configFileError("empty location block");
				checkIndexPages();
				checkErrorPages();
				checkReturn();
				return true;
			} else {
				try {
					KeywordHandler handler = _keywordHandlers.at(keyword);
					if (!(this->*handler)(iss))
						return false;
				} catch (const std::exception& e) {
					return configFileError("invalid keyword in location block: " + keyword);
				}
				empty = false;
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

	std::string getUri() const { return _uri; }
	std::string getRootDir() const { return _rootDir; }
	std::string getCgiExec() const { return _cgiExec; }
	bool getAutoIndex() const { return _autoIndex; }
	std::pair<long, std::string> getReturn() const { return _return; }
	const bool* getAllowedMethod() const { return _allowedMethods; }
	std::map<int, std::string> const& getErrorPages() const { return _errorPages; }
	std::vector<std::string> const& getIndexPages() const { return _indexPages; }

private:
	typedef bool (Location::*KeywordHandler)(std::istringstream&);
	typedef enum Modifier { NONE, REGEX, EXACT } Modifier;
	Modifier _modifier;
	std::string _uri;
	std::string _rootDir;
	std::string _cgiExec;
	bool _autoIndex;
	std::pair<long, std::string> _return;
	bool _allowedMethods[NO_METHOD];
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
		_keywordHandlers["cgi"] = &Location::parseCgi;
	}

	bool parseRoot(std::istringstream& iss) { return ::parseString(iss, _rootDir, "root"); }
	bool parseAutoIndex(std::istringstream& iss) { return ::parseAutoIndex(iss, _autoIndex); }
	bool parseErrorPages(std::istringstream& iss) { return ::parseErrorPages(iss, _errorPages); }
	bool parseIndex(std::istringstream& iss) { return ::parseIndex(iss, _indexPages); }
	bool parseReturn(std::istringstream& iss) { return ::parseReturn(iss, _return); }
	bool parseCgi(std::istringstream& iss) { return ::parseString(iss, _cgiExec, "cgi"); }

	bool parseMethods(std::istringstream& iss) {
		std::string method;
		if (!(iss >> method))
			return configFileError("missing information after limit_except keyword");
		std::fill_n(_allowedMethods, NO_METHOD, false);
		do {
			if (!updateMethod(method))
				return false;
		} while (iss >> method);
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

public:
	void print() const {
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
};
