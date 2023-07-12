#pragma once

#include "webserv.hpp"

extern const std::map<StatusCode, std::string> STATUS_MESSAGES;
extern const std::map<std::string, std::string> MIME_TYPES;

class Response {
public:
	Response(std::string rootDir, bool autoIndex, std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages)
		: _bodyPos(0), _statusCode(STATUS_NONE), _rootDir(rootDir), _autoIndex(autoIndex),
		  _serverErrorPages(errorPages), _indexPages(indexPages), _return(-1, "") {
		initAllowedMethods(_allowedMethods);
		initMethodMap();
	}

	Response(std::string rootDir, std::string uploadDir, bool autoIndex,
			 std::map<int, std::string> const& serverErrorPages,
			 std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages, std::string locationUri,
			 std::pair<long, std::string> redirect, const bool allowedMethods[NO_METHOD],
			 std::string cgiExec)
		: _bodyPos(0), _statusCode(STATUS_NONE), _rootDir(rootDir), _uploadDir(uploadDir),
		  _autoIndex(autoIndex), _serverErrorPages(serverErrorPages), _errorPages(errorPages),
		  _indexPages(indexPages), _locationUri(locationUri), _return(redirect), _cgiExec(cgiExec) {
		for (int i = 0; i < NO_METHOD; ++i) {
			_allowedMethods[i] = allowedMethods[i];
		}
		initMethodMap();
	}

	~Response(){};

	void buildResponse(RequestParsingResult& request) {
		if (request.result == REQUEST_PARSING_FAILURE) {
			buildErrorPage(request, request.statusCode);
		} else if (!_allowedMethods[request.success.method]) {
			buildErrorPage(request, STATUS_METHOD_NOT_ALLOWED);
		} else if (!_cgiExec.empty()) {
			buildCgi(request);
		} else if (_return.first != -1) {
			buildRedirect(request);
		} else {
			MethodHandler handler = _methodHandlers[request.success.method];
			(this->*handler)(request);
		}
		buildStatusLine();
		buildHeader();
	}

	ResponseStatusEnum pushResponseToClient(int fd) {
		std::string line;
		if (_bodyPos == 0) {
			std::cout << GREEN << "=== RESPONSE START ===" << RESET << std::endl;
			if (!pushStringToClient(fd, _statusLine)) {
				return RESPONSE_FAILURE;
			}
			for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
				 it != _headers.end(); it++) {
				line = it->first + ": " + it->second + "\r\n";
				if (!pushStringToClient(fd, line)) {
					return RESPONSE_FAILURE;
				}
			}
			line = "\r\n";
			if (!pushStringToClient(fd, line)) {
				return RESPONSE_FAILURE;
			}
		}
		if (!pushBodyChunkToClient(fd)) {
			return RESPONSE_FAILURE;
		}
		if (_bodyPos == _body.size()) {
			std::cout << GREEN << "\n=== RESPONSE END ===" << RESET << std::endl;
			return RESPONSE_SUCCESS;
		}
		return RESPONSE_PENDING;
	}

private:
	typedef void (Response::*MethodHandler)(RequestParsingResult&);
	std::map<RequestMethod, MethodHandler> _methodHandlers;
	std::string _statusLine;
	std::map<std::string, std::string> _headers;
	std::string _body;
	size_t _bodyPos;
	StatusCode _statusCode;

	std::string _rootDir;
	std::string _uploadDir;
	bool _autoIndex;
	std::map<int, std::string> _serverErrorPages;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::string _locationUri;
	std::pair<long, std::string> _return;
	bool _allowedMethods[NO_METHOD];
	std::string _cgiExec;

	void initMethodMap() {
		_methodHandlers[GET] = &Response::buildGet;
		_methodHandlers[POST] = &Response::buildPost;
		_methodHandlers[DELETE] = &Response::buildDelete;
	}

	void buildGet(RequestParsingResult& request) {
		_statusCode = STATUS_OK;
		if (isDirectory(findFinalUri(request))) {
			handleIndex(request);
		} else {
			buildPage(request);
		}
	}

	void buildPost(RequestParsingResult& request) {
		std::map<std::string, std::string>::const_iterator it =
			request.success.headers.find("content-type");
		if (it == request.success.headers.end()) {
			return buildErrorPage(request, STATUS_BAD_REQUEST);
		} else if (it->second == "application/x-www-form-urlencoded" ||
				   it->second == "multipart/form-data") {
			return buildErrorPage(request, STATUS_UNSUPPORTED_MEDIA_TYPE);
		} else if (!isDirectory(_uploadDir)) {
			return buildErrorPage(request, STATUS_NOT_FOUND);
		}
		std::ofstream ofs(getFileUri(request).c_str());
		if (ofs.fail()) {
			return buildErrorPage(request, STATUS_BAD_REQUEST);
		}
		std::string bodyStr(request.success.body.begin(), request.success.body.end());
		ofs << bodyStr;
		ofs.close();
		_statusCode = STATUS_CREATED;
	}

	void buildDelete(RequestParsingResult& request) {
		std::string uri = getFileUri(request);
		if (isDirectory(uri)) {
			return buildErrorPage(request, STATUS_FORBIDDEN);
		} else if (!isValidFile(uri)) {
			return buildErrorPage(request, STATUS_NOT_FOUND);
		} else if (std::remove(uri.c_str()) == -1) {
			return buildErrorPage(request, STATUS_FORBIDDEN);
		}
		_statusCode = STATUS_NO_CONTENT;
	}

	bool pushStringToClient(int fd, std::string& line) {
		std::cout << GREEN << line << RESET;
		size_t totalSent = 0;
		while (totalSent < line.size()) {
			int curSent = send(fd, line.c_str() + totalSent, line.size() - totalSent, MSG_NOSIGNAL);
			if (curSent <= 0) {
				perrored("send");
				return false;
			}
			totalSent += curSent;
		}
		return true;
	}

	bool pushBodyChunkToClient(int fd) {
		size_t toSend =
			std::min(_body.size() - _bodyPos, static_cast<size_t>(RESPONSE_BUFFER_SIZE));
		size_t sent = send(fd, _body.c_str() + _bodyPos, toSend, MSG_NOSIGNAL);
		if (sent < 0) {
			perrored("send");
			return false;
		}
		std::cout << GREEN << _body.substr(_bodyPos, sent) << RESET;
		_bodyPos += sent;
		return true;
	}

	void buildStatusLine() {
		_statusLine = std::string(HTTP_VERSION) + " " + toString(_statusCode) + " " +
					  STATUS_MESSAGES.find(_statusCode)->second + "\r\n";
	}

	void buildHeader() {
		_headers["date"] = getDate();
		_headers["server"] = SERVER_VERSION;
		_headers["content-length"] = toString(_body.length());
		if (_headers.find("content-type") == _headers.end()) {
			_headers["content-type"] = DEFAULT_CONTENT_TYPE;
		}
		//_headers["connection"] = "close"; TODO
	}

	void buildErrorPage(RequestParsingResult& request, StatusCode statusCode) {
		_statusCode = statusCode;
		std::map<int, std::string>::iterator it = _errorPages.find(_statusCode);
		std::string errorPageUri;
		if (it != _errorPages.end()) {
			errorPageUri = "." + _rootDir + it->second;
		} else {
			it = _serverErrorPages.find(_statusCode);
			errorPageUri = it != _serverErrorPages.end()
							   ? "." + request.virtualServer->getRootDir() + it->second
							   : "./www/error/default_error.html";
		}
		if (!readContent(errorPageUri, _body)) {
			_body = "There was an error while trying to access the specified error page for error "
					"code " +
					toString(_statusCode);
			_headers["content-type"] = "text/plain";
		} else {
			_headers["content-type"] = "text/html";
		}
	}

	void buildPage(RequestParsingResult& request) {
		std::string uri = findFinalUri(request);
		if (!readContent(uri, _body)) {
			return buildErrorPage(request, STATUS_NOT_FOUND);
		}
		std::string extension = getExtension(uri);
		std::map<std::string, std::string>::const_iterator it = MIME_TYPES.find(extension);
		_headers["content-type"] = it != MIME_TYPES.end() ? it->second : DEFAULT_CONTENT_TYPE;
	}

	static void exportEnv(char** env, size_t i, const std::string& key, const std::string& value) {
		env[i] = new char[key.size() + value.size() + 2];
		std::strcpy(env[i], key.c_str());
		env[i][key.size()] = '=';
		std::strcpy(env[i] + key.size() + 1, value.c_str());
	}

	static char** createEnv(char** env, RequestParsingResult& request) {
		std::vector<std::string> envec;
		exportEnv(env, 0, "QUERY_STRING", request.success.query);
		return env;
	}

	void buildCgi(RequestParsingResult& request) {
		int pipefd[2];
		syscall(pipe(pipefd), "pipe");
		pid_t pid = fork();
		syscall(pid, "fork");
		if (pid == 0) {
			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[1]);
			char* strExec = const_cast<char*>(_cgiExec.c_str());
			std::string finalUri = findFinalUri(request);
			char* strScript = const_cast<char*>(finalUri.c_str());
			char* argv[] = {strExec, strScript, NULL};
			char* env[CGI_ENV_SIZE];
			std::memset(env, 0, sizeof(env));
			createEnv(env, request);
			execve(strExec, argv, env);
			perrored("execve");
			exit(EXIT_FAILURE);
		}
		_statusCode = STATUS_OK;
		close(pipefd[1]);
		std::string response = fullRead(pipefd[0]);
		close(pipefd[0]);
		int wstatus;
		wait(&wstatus);
		std::istringstream iss(response);
		std::string line;
		while (std::getline(iss, line) && !line.empty()) {
			size_t colon = line.find(':');
			if (colon != std::string::npos) {
				std::string key = strlower(strtrim(line.substr(0, colon), SPACES));
				std::string value = strtrim(line.substr(colon + 1), SPACES);
				_headers[key] = value;
				if (key == "status") {
					std::istringstream iss(value);
					iss >> value;
					if (value.size() == 3 && isdigit(value[0]) && isdigit(value[1]) &&
						isdigit(value[2])) {
						_statusCode = static_cast<StatusCode>(std::atoi(value.c_str()));
					} else {
						return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
					}
				}
			}
		}
		std::stringstream buffer;
		buffer << iss.rdbuf();
		_body = buffer.str();
		const int exitCode = WIFSIGNALED(wstatus) ? 128 + WTERMSIG(wstatus) : WEXITSTATUS(wstatus);
		if (exitCode != 0) {
			return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
		}
	}

	std::string findFinalUri(RequestParsingResult& request) {
		// note : this function will be called after checking if the requested uri is a file or
		// a directory
		Location* location = request.location;
		std::string uri = request.success.uri;
		// to ensure that the final link will be well formated whether the user put a trailing
		// slash at the end of the location and at the beginning of the uri or not
		if (_rootDir[_rootDir.size() - 1] == '/') {
			_rootDir = _rootDir.substr(0, _rootDir.size() - 1);
		}
		uri = uri.substr(1); // removed: if (uri[0] == '/')
		if (location == NULL) {
			return "." + _rootDir + "/" + uri;
		}
		LocationModifierEnum modifier = location->getModifier();
		if (modifier == DIRECTORY) {
			return "." + _rootDir + "/" + uri.substr(_locationUri.size() - 1);
		} else if (modifier == REGEX) {
			return "." + _rootDir + "/" + uri;
		} else {
			return "." + _rootDir + "/" + getBasename(uri);
		}
	}

	std::string getFileUri(RequestParsingResult& request) {
		std::string filename = getBasename(request.success.uri);
		std::string filepath = _uploadDir[_uploadDir.size() - 1] == '/'
								   ? _uploadDir + filename
								   : _uploadDir + "/" + filename;
		return filepath;
	}

	void handleIndex(RequestParsingResult& request) {
		if (!_indexPages.empty()) {
			std::string filepath;
			for (std::vector<std::string>::iterator it = _indexPages.begin();
				 it != _indexPages.end(); it++) {
				filepath = (*it)[0] == '/' ? findFinalUri(request) + (*it).substr(1)
										   : findFinalUri(request) + *it;
				if (isValidFile(filepath)) {
					std::string indexFile = (*it)[0] == '/' ? (*it).substr(1) : *it;
					request.success.uri += indexFile;
					request.location =
						request.virtualServer->findMatchingLocation(request.success.uri);
					reinitResponseVariables(request);
					buildPage(request);
					return;
				}
			}
		}
		if (_autoIndex) {
			_statusCode = STATUS_OK;
			buildAutoIndexPage(request);
		} else {
			buildErrorPage(request, STATUS_FORBIDDEN);
		}
	}

	void buildRedirect(RequestParsingResult& request) {
		_headers["location"] = _return.second;
		buildErrorPage(request, static_cast<StatusCode>(_return.first));
	}

	void buildAutoIndexPage(RequestParsingResult& request) {
		DIR* dir = opendir(findFinalUri(request).c_str());
		if (dir == NULL) {
			return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
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
		_headers["content-type"] = "text/html";
	}

	void reinitResponseVariables(RequestParsingResult& request) {
		VirtualServer* vs = request.virtualServer;
		if (request.location == NULL) {
			_rootDir = vs->getRootDir();
			_autoIndex = vs->getAutoIndex();
			_serverErrorPages = vs->getErrorPages();
			_locationUri = "";
			_return.first = -1;
			_return.second = "";
			for (int i = 0; i < NO_METHOD; i++) {
				_allowedMethods[i] = true;
			}
			_cgiExec = "";
		} else {
			Location* location = request.location;
			_rootDir = location->getRootDir();
			_autoIndex = location->getAutoIndex();
			_serverErrorPages = vs->getErrorPages();
			_errorPages = location->getErrorPages();
			_indexPages = location->getIndexPages();
			_locationUri = location->getUri();
			_return = location->getReturn();
			for (int i = 0; i < NO_METHOD; i++) {
				_allowedMethods[i] = location->getAllowedMethod()[i];
			}
			_cgiExec = location->getCgiExec();
		}
	}

	std::string getAutoIndexEntry(struct dirent* entry) {
		std::string html = "<tr>\n\
					<td><a href=\"";
		std::string name = static_cast<std::string>(entry->d_name);
		if (entry->d_type == DT_DIR) {
			name += '/';
		}
		html += name;
		html += "\">";
		html += name;
		html += "</a></td>\n\
					</tr>\n";
		return html;
	}
};