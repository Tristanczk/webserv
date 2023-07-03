#pragma once

#include "Location.hpp"
#include "webserv.hpp"
#include <string>

class Response {
public:
	// constructor in case of non matching location block, there won't be a locationUri, a redirect,
	// allowedMethods or link to cgiExec as they are exclusively defined in the location block
	Response(std::string rootDir, bool autoIndex, std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages)
		: _rootDir(rootDir), _autoIndex(autoIndex), _errorPages(errorPages),
		  _indexPages(indexPages), _return(-1, "") {
		std::fill_n(_allowedMethods, NO_METHOD, true);
		initKeywordMap();
		initStatusMessageMap();
		// TODO remove void
		(void)_allowedMethods;
		(void)_autoIndex;
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
		initStatusMessageMap();
		// TODO remove void
		(void)_allowedMethods;
		(void)_autoIndex;
	}

	~Response(){};

	void buildResponse(RequestParsingResult& request) {
		if (request.result == REQUEST_PARSING_FAILURE) {
			_statusCode = request.statusCode;
			buildErrorPage();
			buildStatusLine();
			buildHeader();
		} else if (request.success.method == GET) {
			_statusCode = SUCCESS_OK;
			buildGet(request);
		}
	}

	bool pushResponseToClient(int fd) {
		std::string line;
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
		return true;
	}

private:
	typedef void (Response::*KeywordHandler)(RequestParsingResult&);
	std::map<RequestMethod, KeywordHandler> _keywordHandlers;
	std::string _statusLine;
	std::map<std::string, std::string> _headers;
	std::vector<char> _tmpBody; // TODO remove?
	std::string _body;
	std::string _bodyType;
	StatusCode _statusCode;

	std::map<StatusCode, std::string> _statusMessages;

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
		// no need to check if it is a directory as we only want to know if the requested uri has
		// the form of a directory, not that it is a necessarily valid directory
		if (isDirectory(findFinalUri(request))) {
			std::cout << RED << "Index handling" << RESET << std::endl;
			handleIndex(request);
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
		(void)request;
		return;
	}

	bool pushStringToClient(int fd, std::string& line) {
		// TODO tout ca me parait tres louche et potentiellement bloquant
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
		_statusLine =
			"HTTP/1.1 " + toString(_statusCode) + " " + _statusMessages[_statusCode] + "\r\n";
	}

	void buildHeader() {
		_headers["Date"] = getDate();
		_headers["Server"] = "webserv/4.2";
		_headers["Content-Length"] = toString(_body.length());
		if (!_bodyType.empty())
			_headers["Content-Type"] = _bodyType;
		//_headers["Connection"] = "close";
	}

	void buildErrorPage() {
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
		if (_cgiScript.empty()) {
			if (!_allowedMethods[request.success.method]) {
				_statusCode = CLIENT_METHOD_NOT_ALLOWED;
				return false;
			}
			std::string uri = findFinalUri(request);
			std::cout << "final uri: " << uri << std::endl;
			if (!readContent(uri, _body)) {
				_statusCode = CLIENT_NOT_FOUND;
				return false;
			}
			std::string extension = getExtension(uri);
			if (extension.empty()) {
				_bodyType = "application/octet-stream";
			}
			_bodyType = "text/html"; // TODO MIME types when maps are definitely implemented
			return true;
		} else {
			return buildCgiPage(request);
		}
	}

	bool buildCgiPage(RequestParsingResult& request) {
		(void)request;
		char* strExec = const_cast<char*>(_cgiExec.c_str());
		const std::string dotSlash = "." + _cgiScript;
		char* strScript = const_cast<char*>(dotSlash.c_str());
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
		html += entry->d_type == DT_DIR ? static_cast<std::string>(entry->d_name) + "/"
										: static_cast<std::string>(entry->d_name);
		html += "\">";
		html += entry->d_type == DT_DIR ? static_cast<std::string>(entry->d_name) + "/"
										: static_cast<std::string>(entry->d_name);
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

	// TODO: see how to implement to be more efficient and not regenerate for every response
	void initStatusMessageMap() {
		_statusMessages[INFORMATIONAL_CONTINUE] = "Continue";
		_statusMessages[INFORMATIONAL_SWITCHING_PROTOCOLS] = "Switching Protocols";
		_statusMessages[INFORMATIONAL_PROCESSING] = "Processing";
		_statusMessages[INFORMATIONAL_EARLY_HINTS] = "Early Hints";
		_statusMessages[SUCCESS_OK] = "OK";
		_statusMessages[SUCCESS_CREATED] = "Created";
		_statusMessages[SUCCESS_ACCEPTED] = "Accepted";
		_statusMessages[SUCCESS_NON_AUTHORITATIVE_INFORMATION] = "Non-Authoritative Information";
		_statusMessages[SUCCESS_NO_CONTENT] = "No Content";
		_statusMessages[SUCCESS_RESET_CONTENT] = "Reset Content";
		_statusMessages[SUCCESS_PARTIAL_CONTENT] = "Partial Content";
		_statusMessages[SUCCESS_MULTI_STATUS] = "Multi-Status";
		_statusMessages[SUCCESS_ALREADY_REPORTED] = "Already Reported";
		_statusMessages[SUCCESS_IM_USED] = "IM Used";
		_statusMessages[REDIRECTION_MULTIPLE_CHOICES] = "Multiple Choices";
		_statusMessages[REDIRECTION_MOVED_PERMANENTLY] = "Moved Permanently";
		_statusMessages[REDIRECTION_FOUND] = "Found";
		_statusMessages[REDIRECTION_SEE_OTHER] = "See Other";
		_statusMessages[REDIRECTION_NOT_MODIFIED] = "Not Modified";
		_statusMessages[REDIRECTION_USE_PROXY] = "Use Proxy";
		_statusMessages[REDIRECTION_TEMPORARY_REDIRECT] = "Temporary Redirect";
		_statusMessages[REDIRECTION_PERMANENT_REDIRECT] = "Permanent Redirect";
		_statusMessages[CLIENT_BAD_REQUEST] = "Bad Request";
		_statusMessages[CLIENT_UNAUTHORIZED] = "Unauthorized";
		_statusMessages[CLIENT_PAYMENT_REQUIRED] = "Payment Required";
		_statusMessages[CLIENT_FORBIDDEN] = "Forbidden";
		_statusMessages[CLIENT_NOT_FOUND] = "Not Found";
		_statusMessages[CLIENT_METHOD_NOT_ALLOWED] = "Method Not Allowed";
		_statusMessages[CLIENT_NOT_ACCEPTABLE] = "Not Acceptable";
		_statusMessages[CLIENT_PROXY_AUTHENTICATION_REQUIRED] = "Proxy Authentication Required";
		_statusMessages[CLIENT_REQUEST_TIMEOUT] = "Request Timeout";
		_statusMessages[CLIENT_CONFLICT] = "Conflict";
		_statusMessages[CLIENT_GONE] = "Gone";
		_statusMessages[CLIENT_LENGTH_REQUIRED] = "Length Required";
		_statusMessages[CLIENT_PRECONDITION_FAILED] = "Precondition Failed";
		_statusMessages[CLIENT_PAYLOAD_TOO_LARGE] = "Payload Too Large";
		_statusMessages[CLIENT_URI_TOO_LONG] = "URI Too Long";
		_statusMessages[CLIENT_UNSUPPORTED_MEDIA_TYPE] = "Unsupported Media Type";
		_statusMessages[CLIENT_RANGE_NOT_SATISFIABLE] = "Range Not Satisfiable";
		_statusMessages[CLIENT_EXPECTATION_FAILED] = "Expectation Failed";
		_statusMessages[CLIENT_IM_A_TEAPOT] = "I'm a teapot";
		_statusMessages[CLIENT_MISDIRECTED_REQUEST] = "Misdirected Request";
		_statusMessages[CLIENT_UNPROCESSABLE_ENTITY] = "Unprocessable Entity";
		_statusMessages[CLIENT_LOCKED] = "Locked";
		_statusMessages[CLIENT_FAILED_DEPENDENCY] = "Failed Dependency";
		_statusMessages[CLIENT_TOO_EARLY] = "Too Early";
		_statusMessages[CLIENT_UPGRADE_REQUIRED] = "Upgrade Required";
		_statusMessages[CLIENT_PRECONDITION_REQUIRED] = "Precondition Required";
		_statusMessages[CLIENT_TOO_MANY_REQUESTS] = "Too Many Requests";
		_statusMessages[CLIENT_REQUEST_HEADER_FIELDS_TOO_LARGE] = "Request Header Fields Too Large";
		_statusMessages[CLIENT_UNAVAILABLE_FOR_LEGAL_REASONS] = "Unavailable For Legal Reasons";
		_statusMessages[SERVER_INTERNAL_SERVER_ERROR] = "Internal Server Error";
		_statusMessages[SERVER_NOT_IMPLEMENTED] = "Not Implemented";
		_statusMessages[SERVER_BAD_GATEWAY] = "Bad Gateway";
		_statusMessages[SERVER_SERVICE_UNAVAILABLE] = "Service Unavailable";
		_statusMessages[SERVER_GATEWAY_TIMEOUT] = "Gateway Timeout";
		_statusMessages[SERVER_HTTP_VERSION_NOT_SUPPORTED] = "HTTP Version Not Supported";
		_statusMessages[SERVER_VARIANT_ALSO_NEGOTIATES] = "Variant Also Negotiates";
		_statusMessages[SERVER_INSUFFICIENT_STORAGE] = "Insufficient Storage";
		_statusMessages[SERVER_LOOP_DETECTED] = "Loop Detected";
		_statusMessages[SERVER_NOT_EXTENDED] = "Not Extended";
		_statusMessages[SERVER_NETWORK_AUTHENTICATION_REQUIRED] = "Network Authentication Required";
	}
};