#pragma once

#include "webserv.hpp"

class Location {
public:
	Location(const std::string& rootDir, bool autoIndex,
			 const std::vector<std::string>& serverIndexPages,
			 const std::pair<long, std::string>& serverReturn)
		: _modifier(DIRECTORY), _rootDir(rootDir), _uploadDir(""), _autoIndex(autoIndex),
		  _return(-1, ""), _serverIndexPages(serverIndexPages), _serverReturn(serverReturn) {
		initKeywordMap();
		initAllowedMethods(_allowedMethods);
	}

	~Location(){};

	bool initUri(std::istringstream& iss) {
		std::string modifier, uri, check;
		if (!(iss >> modifier >> uri))
			return configFileError(ERROR_LOCATION_FORMAT);
		if (uri == "{") {
			_uri = modifier + (modifier[modifier.size() - 1] == '/' ? "" : "/");
			_modifier = DIRECTORY;
		} else {
			_uri = uri;
			if (modifier == "~")
				_modifier = REGEX;
			else if (modifier == "=")
				_modifier = EXACT;
			else
				return configFileError("invalid modifier for location: " + modifier);
			if (!(iss >> check))
				return configFileError(ERROR_LOCATION_FORMAT);
		}
		if (_modifier != REGEX && _uri[0] != '/') {
			return configFileError(
				"location uri must start with a slash when not using a regex modifier: " + _uri);
			_rootDir += _uri; // TODO Tristan please check
		}
		if (iss >> check)
			return configFileError(ERROR_LOCATION_FORMAT);
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
				checkReturn();
				return checkUpload();
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
		case DIRECTORY:
			return comparePrefix(_uri, requestPath);
		case REGEX:
			return doesRegexMatch(_uri.c_str(), requestPath.c_str()) ? LOCATION_MATCH_REGEX
																	 : LOCATION_MATCH_NONE;
		case EXACT:
			return requestPath == _uri ? LOCATION_MATCH_EXACT : LOCATION_MATCH_NONE;
		}
	}

	LocationModifierEnum getModifier() const { return _modifier; }
	std::string getUri() const { return _uri; }
	std::string getRootDir() const { return _rootDir; }
	std::string getUploadDir() const { return _uploadDir; }
	std::string getCgiExec() const { return _cgiExec; }
	bool getAutoIndex() const { return _autoIndex; }
	std::pair<long, std::string> getReturn() const { return _return; }
	const bool* getAllowedMethod() const { return _allowedMethods; }
	std::map<int, std::string> const& getErrorPages() const { return _errorPages; }
	std::vector<std::string> const& getIndexPages() const { return _indexPages; }

private:
	typedef bool (Location::*KeywordHandler)(std::istringstream&);

	LocationModifierEnum _modifier;
	std::string _uri;
	std::string _rootDir;
	std::string _uploadDir;
	std::string _cgiExec;
	bool _autoIndex;
	std::pair<long, std::string> _return;
	bool _allowedMethods[NO_METHOD];
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	const std::vector<std::string>& _serverIndexPages;
	const std::pair<long, std::string>& _serverReturn;
	std::map<std::string, KeywordHandler> _keywordHandlers;

	void initKeywordMap() {
		_keywordHandlers["root"] = &Location::parseRoot;
		_keywordHandlers["upload_directory"] = &Location::parseUploadDir;
		_keywordHandlers["autoindex"] = &Location::parseAutoIndex;
		_keywordHandlers["error_page"] = &Location::parseErrorPages;
		_keywordHandlers["index"] = &Location::parseIndex;
		_keywordHandlers["return"] = &Location::parseReturn;
		_keywordHandlers["limit_except"] = &Location::parseLimitExcept;
		_keywordHandlers["cgi"] = &Location::parseCgi;
	}

	bool parseAutoIndex(std::istringstream& iss) { return ::parseAutoIndex(iss, _autoIndex); }
	bool parseErrorPages(std::istringstream& iss) { return ::parseErrorPages(iss, _errorPages); }
	bool parseIndex(std::istringstream& iss) { return ::parseIndex(iss, _indexPages); }
	bool parseReturn(std::istringstream& iss) { return ::parseReturn(iss, _return); }

	bool parseCgi(std::istringstream& iss) {
		if (!(iss >> _cgiExec))
			return configFileError("missing information after cgi keyword");
		if (!iss.eof())
			return configFileError("too many arguments after cgi keyword");
		if (!validateUri(_cgiExec, "cgi"))
			return false;
		struct stat sb;
		if (stat(_cgiExec.c_str(), &sb) != 0)
			return configFileError("cgi interpreter not found: " + _cgiExec);
		if (!S_ISREG(sb.st_mode) || access(_cgiExec.c_str(), X_OK) != 0)
			return configFileError("cgi interpreter is not executable: " + _cgiExec);
		return true;
	}

	bool parseRoot(std::istringstream& iss) {
		return ::parseDirectory(iss, _rootDir, "location", "root") &&
			   validateUri(_rootDir, "location root");
	}

	bool parseUploadDir(std::istringstream& iss) {
		return ::parseDirectory(iss, _uploadDir, "location", "upload_directory") &&
			   validateUri(_uploadDir, "location upload_directory");
	}

	bool parseLimitExcept(std::istringstream& iss) {
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
		if (_indexPages.empty())
			_indexPages = _serverIndexPages;
	}

	void checkReturn() {
		if (_return.first == -1 && _serverReturn.first != -1)
			_return = _serverReturn;
	}

	bool checkUpload() {
		if (_allowedMethods[POST] && _uploadDir.empty())
			return configFileError("upload_directory not specified for POST method");
		if (_allowedMethods[DELETE] && _uploadDir.empty())
			return configFileError("upload_directory not specified for DELETE method");
		if (!_uploadDir.empty() && !_allowedMethods[POST] && !_allowedMethods[DELETE])
			return configFileError(
				"upload_directory specified but POST or DELETE methods not allowed");
		return true;
	}
};
