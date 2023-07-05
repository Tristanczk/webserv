#pragma once

#include "Location.hpp"
#include "webserv.hpp"
#include <string>

extern std::map<StatusCode, std::string> STATUS_MESSAGES;
extern std::map<std::string, std::string> MIME_TYPES;

class Response {
public:
	// constructor in case of non matching location block, there won't be a locationUri, a redirect,
	// allowedMethods or link to cgiExec as they are exclusively defined in the location block
	// TODO: no delete as allowed method
	Response(std::string rootDir, bool autoIndex, std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages)
		: _rootDir(rootDir), _autoIndex(autoIndex), _errorPages(errorPages),
		  _indexPages(indexPages), _return(-1, "") {
		std::fill_n(_allowedMethods, NO_METHOD, true);
		initKeywordMap();
	}

	Response(std::string rootDir, bool autoIndex, std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages, std::string locationUri,
			 std::pair<long, std::string> redirect, const bool allowedMethods[NO_METHOD],
			 std::string cgiExec, std::string cgiScript)
		: _rootDir(rootDir), _autoIndex(autoIndex), _errorPages(errorPages),
		  _indexPages(indexPages), _locationUri(locationUri), _return(redirect), _cgiExec(cgiExec),
		  _cgiScript(cgiScript) {
		for (int i = 0; i < NO_METHOD; ++i)
			_allowedMethods[i] = allowedMethods[i];
		initKeywordMap();
	}

	~Response(){};

	void buildResponse(RequestParsingResult& request) {
		if (request.result == REQUEST_PARSING_FAILURE) {
			_statusCode = request.statusCode;
			buildErrorPage();
			buildStatusLine();
			buildHeader();
		} else if (!_allowedMethods[request.success.method]) {
			_statusCode = CLIENT_METHOD_NOT_ALLOWED;
			buildErrorPage();
			buildStatusLine();
			buildHeader();
		} else {
			KeywordHandler handler = _keywordHandlers[request.success.method];
			(this->*handler)(request);
		}
	}

	bool pushResponseToClient(int fd) {
		std::string line;
		std::cout << "=== RESPONSE START ===" << std::endl;
		if (!pushStringToClient(fd, _statusLine))
			return false;
		for (std::map<std::string, std::string>::iterator it = _headers.begin();
			 it != _headers.end(); it++) {
			line = it->first + ": " + it->second + "\r\n";
			if (!pushStringToClient(fd, line))
				return false;
		}
		line = "\r\n";
		if (!pushStringToClient(fd, line))
			return false;
		if (!pushStringToClient(fd, _body))
			return false;
		std::cout << "=== RESPONSE END ===" << std::endl;
		return true;
	}

private:
	typedef void (Response::*KeywordHandler)(RequestParsingResult&);
	std::map<RequestMethod, KeywordHandler> _keywordHandlers;
	std::string _statusLine;
	std::map<std::string, std::string> _headers;
	std::string _body;
	std::string _bodyType;
	StatusCode _statusCode;

	// these variables will be extracted from the correct location or from the correct virtual
	// server if needed
	std::string _rootDir;
	bool _autoIndex;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::string _locationUri;
	std::pair<long, std::string> _return;
	bool _allowedMethods[NO_METHOD];
	std::string _cgiExec;
	std::string _cgiScript;

	void initKeywordMap() {
		_keywordHandlers[GET] = &Response::buildGet;
		_keywordHandlers[POST] = &Response::buildPost;
		_keywordHandlers[DELETE] = &Response::buildDelete;
	}

	// function templates
	void buildGet(RequestParsingResult& request) {
		_statusCode = SUCCESS_OK;
		// TODO: how to handle the fact that a get /error/ and a get /error won't necessarily have
		// the same location ? and we need to know the location in order to know if the directory
		// exists where we want to seach it I suggest we go back to handling only links that end by
		// / as directory as it is more in line with the project prerequisites
		if (isDirectory(findFinalUri(request))) {
			std::cout << RED << "Index handling" << RESET << std::endl;
			handleIndex(request);
			return;
		}
		if (_return.first != -1) {
			handleRedirect();
			return;
		}
		if (!buildPage(request))
			buildErrorPage();
		buildStatusLine();
		buildHeader();
		return;
	}

	void buildPost(RequestParsingResult& request) {
		(void)request;
		return;
	}

	void buildDelete(RequestParsingResult& request) {
		std::string uri = findFinalUri(request);
		bool error = false;
		if (isDirectory(uri)) {
			_statusCode = CLIENT_FORBIDDEN;
			error = true;
		} else if (!isValidFile(uri)) {
			_statusCode = CLIENT_NOT_FOUND;
			error = true;
		} else {
			if (remove(uri.c_str()) == -1) {
				_statusCode = CLIENT_FORBIDDEN;
				error = true;
			} else {
				_statusCode = SUCCESS_NO_CONTENT;
			}
		}
		if (error)
			buildErrorPage();
		buildStatusLine();
		buildHeader();
	}

	bool pushStringToClient(int fd, std::string& line) {
		// TODO tout ca me parait tres louche et potentiellement bloquant
		std::cout << strtrim(line, "\r\n") << std::endl;
		size_t sent = 0;
		while (sent < line.size()) {
			int cur_sent = send(fd, line.c_str() + sent, line.size() - sent, MSG_NOSIGNAL);
			if (cur_sent <= 0) {
				std::cerr << "Error: send failed" << std::endl;
				return false;
			}
			sent += cur_sent;
		}
		return true;
	}

	void buildStatusLine() {
		_statusLine = std::string(HTTP_VERSION) + " " + toString(_statusCode) + " " +
					  STATUS_MESSAGES[_statusCode] + "\r\n";
	}

	void buildHeader() {
		_headers["Date"] = getDate();
		_headers["Server"] = SERVER_VERSION;
		_headers["Content-Length"] = toString(_body.length());
		if (!_bodyType.empty())
			_headers["Content-Type"] = _bodyType;
		//_headers["Connection"] = "close";
	}

	void buildErrorPage() {
		std::cout << _statusCode << std::endl;
		std::map<int, std::string>::iterator it = _errorPages.find(_statusCode);
		std::string errorPageUri = it != _errorPages.end() ? "." + _rootDir + it->second
														   : "./www/error/default_error.html";
		std::cout << errorPageUri << std::endl;
		if (!readContent(errorPageUri, _body)) {
			// TODO return 500 Internal Server Error instead maybe
			_body = "There was an error while trying to access the specified error page for error "
					"code " +
					toString(_statusCode);
			_bodyType = "text/plain";
		} else
			_bodyType = "text/html";
	}

	bool buildPage(RequestParsingResult& request) {
		std::string uri = findFinalUri(request);
		std::cout << "final uri: " << uri << std::endl;
		if (!readContent(uri, _body)) {
			_statusCode = CLIENT_NOT_FOUND;
			return false;
		}
		std::string extension = getExtension(uri);
		std::map<std::string, std::string>::iterator it = MIME_TYPES.find(extension);
		_bodyType = it != MIME_TYPES.end() ? it->second : "application/octet-stream";
		return true;
	}

	bool buildCgiPage(RequestParsingResult& request) {
		(void)request;
		char* strExec = const_cast<char*>(_cgiExec.c_str());
		const std::string dotSlash = "." + _cgiScript;
		char* strScript = const_cast<char*>(dotSlash.c_str());
		std::cout << strExec << " " << strScript << std::endl;
		int pipefd[2];
		syscall(pipe(pipefd), "pipe");
		pid_t pid = fork();
		syscall(pid, "fork");
		if (pid == 0) {
			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[1]);
			char* argv[] = {strExec, strScript, NULL};
			execve(strExec, argv, (char* const[]){NULL});
			std::perror("execve");
			exit(EXIT_FAILURE);
		}
		close(pipefd[1]);
		std::string response = fullRead(pipefd[0]);
		close(pipefd[0]);
		std::cout << "=== CGI RESPONSE BEGIN (" << response.size() << ") ===" << std::endl;
		std::cout << strtrim(response, "\r\n") << std::endl;
		std::cout << "=== CGI RESPONSE END ===" << std::endl;
		int wstatus;
		wait(&wstatus);
		const int exitCode = WIFSIGNALED(wstatus) ? 128 + WTERMSIG(wstatus) : WEXITSTATUS(wstatus);
		if (exitCode == 0) {
			_statusCode = SUCCESS_OK;
			return true;
		} else {
			std::cerr << RED << _cgiExec << " " << dotSlash << " failed with exit code " << exitCode
					  << RESET << std::endl;
			_statusCode = SERVER_INTERNAL_SERVER_ERROR;
			return false;
		}
	}

	std::string findFinalUri(RequestParsingResult& request) {
		// note : this function will be called after checking if the requested uri is a file or a
		// directory
		Location* location = request.location;
		std::string uri = request.success.uri;
		// to ensure that the final link will be well formated whether the user put a trailing slash
		// at the end of the location and at the beginning of the uri or not
		if (_rootDir[_rootDir.size() - 1] == '/')
			_rootDir = _rootDir.substr(0, _rootDir.size() - 1);
		if (uri[0] == '/')
			uri = uri.substr(1);
		if (location == NULL)
			return "." + _rootDir + "/" + uri;
		LocationModifierEnum modifier = location->getModifier();
		if (modifier == NONE)
			return "." + _rootDir + "/" + uri.substr(_locationUri.size() - 1);
		else if (modifier == REGEX) {
			// in case of a matching regex, we append the whole path to the root directory
			return "." + _rootDir + "/" + uri;
		} else {
			// in case of an exact match, we append only the file name to the root directory
			return "." + _rootDir + "/" + getBasename(uri);
		}
	}
	// TODO : should we handle it as a redirection and let the client do another request rather than
	// displaying the index page directly ourselves ?
	void handleIndex(RequestParsingResult& request) {
		bool validIndexFile = false;
		std::string indexFile;
		if (!_indexPages.empty()) {
			std::string filepath;
			for (std::vector<std::string>::iterator it = _indexPages.begin();
				 it != _indexPages.end(); it++) {
				if (_rootDir[_rootDir.size() - 1] == '/')
					_rootDir = _rootDir.substr(0, _rootDir.size() - 1);
				filepath = (*it)[0] == '/' ? findFinalUri(request) + (*it).substr(1)
										   : findFinalUri(request) + *it;
				std::cout << "filepath: " << filepath << std::endl;
				if (isValidFile(filepath)) {
					std::cout << "valid index file: " << *it << std::endl;
					validIndexFile = true;
					indexFile = (*it)[0] == '/' ? (*it).substr(1) : *it;
					break;
				}
				std::cout << "invalid index file: " << *it << std::endl;
			}
			if (validIndexFile) {
				request.success.uri = request.success.uri + indexFile;
				request.location = request.virtualServer->findMatchingLocation(request.success.uri);
				reinitResponseVariables(request);
				if (!buildPage(request))
					buildErrorPage();
				buildStatusLine();
				buildHeader();
				return;
			}
		}
		if (!validIndexFile && _autoIndex) {
			_statusCode = SUCCESS_OK;
			buildAutoIndexPage(request);
			buildStatusLine();
			buildHeader();
			return;
		}
		_statusCode = CLIENT_FORBIDDEN;
		buildErrorPage();
		buildStatusLine();
		buildHeader();
		return;
	}

	void handleRedirect() {
		_statusCode = static_cast<StatusCode>(_return.first);
		buildErrorPage();
		buildStatusLine();
		buildHeader();
		_headers["Location"] = _return.second;
		return;
	}

	void buildAutoIndexPage(RequestParsingResult& request) {
		DIR* dir = opendir(findFinalUri(request).c_str());
		if (dir == NULL) {
			_statusCode = SERVER_INTERNAL_SERVER_ERROR;
			buildErrorPage();
			buildStatusLine();
			buildHeader();
			return;
		}
		_body = "<head>\n"
				"    <title>Index of " +
				request.success.uri +
				"</title>\n"
				"    <style>\n"
				"        body {\n"
				"            font-family: Arial, sans-serif;\n"
				"            margin: 20px;\n"
				"        }\n"
				"\n"
				"        table {\n"
				"            width: 100%;\n"
				"            border-collapse: collapse;\n"
				"        }\n"
				"\n"
				"        th,\n"
				"        td {\n"
				"            padding: 8px;\n"
				"            text-align: left;\n"
				"            border-bottom: 1px solid #ddd;\n"
				"        }\n"
				"\n"
				"        th {\n"
				"            background-color: #f2f2f2;\n"
				"        }\n"
				"    </style>\n"
				"</head>";
		_body += "<body>\n\
				<h1>Index of " +
				 request.success.uri + "</h1>\n\
				<table>\n\
				<thead>\n\
				<tr>\n\
				<th>Name</th>\n\
				</tr>\n\
				</thead >\n\
				<tbody>\n ";
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			_body += getAutoIndexEntry(entry);
		}
		_body += "</tbody>\n\
				</table>\n\
				</body>\n\
				</html>\n";
		_bodyType = "text/html";
	}

	void reinitResponseVariables(RequestParsingResult& request) {
		if (request.location == NULL) {
			VirtualServer* vs = request.virtualServer;
			_rootDir = vs->getRootDir();
			_autoIndex = vs->getAutoIndex();
			_errorPages = vs->getErrorPages();
			_indexPages = vs->getIndexPages();
			_locationUri = "";
			_return.first = -1;
			_return.second = "";
			for (int i = 0; i < NO_METHOD; i++)
				_allowedMethods[i] = true;
			_cgiExec = "";
			_cgiScript = "";
		} else {
			Location* location = request.location;
			_rootDir = location->getRootDir();
			_autoIndex = location->getAutoIndex();
			_errorPages = location->getErrorPages();
			_indexPages = location->getIndexPages();
			_locationUri = location->getUri();
			_return = location->getReturn();
			for (int i = 0; i < NO_METHOD; i++)
				_allowedMethods[i] = location->getAllowedMethod()[i];
			_cgiExec = location->getCgiExec();
			_cgiScript = location->getCgiScript();
		}
	}

	std::string getAutoIndexEntry(struct dirent* entry) {
		std::string html = "<tr>\n\
					<td><a href=\"";
		std::string name = static_cast<std::string>(entry->d_name);
		if (entry->d_type == DT_DIR)
			name += '/';
		html += name;
		html += "\">";
		html += name;
		html += "</a></td>\n\
					</tr>\n";
		return html;
	}

	// what are the type of things we need to handle to build a response?
	//  1. status line:
	//  	- HTTP version (will always be 1.1)
	//  	- status code (if error when parsing request, use that error code, otherwise check if
	//  the request can be answered, if ok code will be 200)
	//  	- status message (what do we put here?)
	//  2. headers: what are the field we need ?
	//  	- Content-Length (if body is empty, set to 0)
	//  	- Content-Type (if body is empty, set to text/html ?)
	//  	- Date (use current date)
	//  	- Server (use server name)
	//  	- Connection : at first we manade only connection: close, we will see if we handle the
	//  keep-alive mode later
	//  3. body:
	//		- if there is an error code, we need to build a body with the error message based on the
	// correct error page, specific error page or default error page
	//  	- content of the html pages
	//  	- if it is a php page, do we need to convert it to html before sending it ? I think yes
	//  but not sure
	//		- handle differently whether the uri is a directory or a file
	//		- if it is a directory, we need to handle it according to the index and autoindex
	// options
};