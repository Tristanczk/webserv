#pragma once

#include "webserv.hpp"

extern const std::map<StatusCode, std::string> STATUS_MESSAGES;
extern const std::map<std::string, std::string> MIME_TYPES;
extern const std::set<std::string> CGI_NO_TRANSMISSION;

class Response {
public:
	Response(RequestMethod method, std::string rootDir, bool autoIndex,
			 std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages)
		: _bodyPos(0), _statusCode(STATUS_NONE), _method(method), _rootDir(rootDir),
		  _autoIndex(autoIndex), _serverErrorPages(errorPages), _indexPages(indexPages),
		  _return(-1, "") {
		initAllowedMethods(_allowedMethods);
		initMethodMap();
	}

	Response(RequestMethod method, std::string rootDir, std::string uploadDir, bool autoIndex,
			 std::map<int, std::string> const& serverErrorPages,
			 std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages, std::string locationUri,
			 std::pair<long, std::string> redirect, const bool allowedMethods[NO_METHOD],
			 std::string cgiExec)
		: _bodyPos(0), _statusCode(STATUS_NONE), _method(method), _rootDir(rootDir),
		  _uploadDir(uploadDir), _autoIndex(autoIndex), _serverErrorPages(serverErrorPages),
		  _errorPages(errorPages), _indexPages(indexPages), _locationUri(locationUri),
		  _return(redirect), _cgiExec(cgiExec) {
		std::copy(allowedMethods, allowedMethods + NO_METHOD, _allowedMethods);
		initMethodMap();
	}

	~Response(){};

	void buildResponse(RequestParsingResult& request) {
		if (request.result == REQUEST_PARSING_FAILURE) {
			buildErrorPage(request, request.statusCode);
		} else if (!_cgiExec.empty()) {
			buildCgi(request);
		} else if (!_allowedMethods[request.success.method]) {
			buildErrorPage(request, STATUS_METHOD_NOT_ALLOWED);
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
			if (DEBUG) {
				std::cout << GREEN << "=== RESPONSE START ===" << RESET << std::endl;
			}
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
			for (std::vector<std::string>::const_iterator it = _setCookies.begin();
				 it != _setCookies.end(); it++) {
				line = "set-cookie: " + *it + "\r\n";
				if (!pushStringToClient(fd, line)) {
					return RESPONSE_FAILURE;
				}
			}
			line = "\r\n";
			if (!pushStringToClient(fd, line)) {
				return RESPONSE_FAILURE;
			}
		}
		if (_method == HEAD) {
			if (DEBUG) {
				std::cout << GREEN << "\n=== RESPONSE END ===" << RESET << std::endl;
			}
			return RESPONSE_SUCCESS;
		}
		if (!pushBodyChunkToClient(fd)) {
			return RESPONSE_FAILURE;
		}
		if (_bodyPos == _body.size()) {
			if (DEBUG) {
				std::cout << GREEN << "\n=== RESPONSE END ===" << RESET << std::endl;
			}
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
	RequestMethod _method;

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
	std::vector<std::string> _setCookies;

	void initMethodMap() {
		_methodHandlers[GET] = &Response::buildGet;
		_methodHandlers[POST] = &Response::buildPost;
		_methodHandlers[DELETE] = &Response::buildDelete;
		_methodHandlers[HEAD] = &Response::buildGet;
	}

	void buildGet(RequestParsingResult& request) {
		_statusCode = STATUS_OK;
		if (isDirectory(findFinalUri(request.success.uri, _rootDir, request.location))) {
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
		} else if (!isDirectory("." + _uploadDir)) {
			return buildErrorPage(request, STATUS_NOT_FOUND);
		}
		const std::string fileName = "." + getFileUri(request);
		bool existed = access(fileName.c_str(), F_OK) == 0;
		std::ofstream ofs(fileName.c_str());
		if (ofs.fail()) {
			return buildErrorPage(request, STATUS_BAD_REQUEST);
		}
		std::string bodyStr(request.success.body.begin(), request.success.body.end());
		ofs << bodyStr;
		if (ofs.fail()) {
			if (!existed) {
				std::remove(fileName.c_str());
			}
			ofs.close();
			return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
		}
		ofs.close();
		_statusCode = STATUS_CREATED;
	}

	void buildDelete(RequestParsingResult& request) {
		std::string uri = '.' + getFileUri(request);
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
		if (DEBUG) {
			std::cout << GREEN << line << RESET;
		}
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
		ssize_t sent = send(fd, _body.c_str() + _bodyPos, toSend, MSG_NOSIGNAL);
		if (sent < 0) {
			perrored("send");
			return false;
		}
		if (DEBUG) {
			std::cout << GREEN << _body.substr(_bodyPos, sent) << RESET;
		}
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
		_headers["content-length"] = toString(_body.size());
		if (_headers.find("content-type") == _headers.end()) {
			_headers["content-type"] = DEFAULT_CONTENT_TYPE;
		}
		_headers["connection"] = "close";
	}

	void buildErrorPage(RequestParsingResult& request, StatusCode statusCode) {
		_statusCode = statusCode;
		std::map<int, std::string>::iterator locationIt = _errorPages.find(_statusCode);
		std::map<int, std::string>::iterator serverIt = _serverErrorPages.find(_statusCode);
		std::string errorPageUri =
			locationIt != _errorPages.end() ? "." + _rootDir + locationIt->second
			: serverIt != _serverErrorPages.end()
				? "." + request.virtualServer->getRootDir() + serverIt->second
				: "";
		if (errorPageUri.empty() || !readContent(errorPageUri, _body)) {
			std::map<StatusCode, std::string>::const_iterator it =
				STATUS_MESSAGES.find(_statusCode);
			std::string codeString = toString(_statusCode);
			std::string title = it != STATUS_MESSAGES.end() ? codeString + " " + it->second
															: "Unknown error " + codeString;
			_body = "<!DOCTYPE html>\n"
					"<html lang=\"en\">\n"
					"\n"
					"<head>\n"
					"\t<meta charset=\"UTF-8\">\n"
					"\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
					"\t<title>" +
					title +
					"</title>\n"
					"\t<style>\n"
					"\t\tbody {\n"
					"\t\t\tbackground-color: #f0f0f0;\n"
					"\t\t\tfont-family: Arial, sans-serif;\n"
					"\t\t}\n"
					"\n"
					"\t\t.container {\n"
					"\t\t\twidth: 80%;\n"
					"\t\t\tmargin: auto;\n"
					"\t\t\ttext-align: center;\n"
					"\t\t\tpadding-top: 20%;\n"
					"\t\t}\n"
					"\n"
					"\t\th1 {\n"
					"\t\t\tcolor: #333;\n"
					"\t\t}\n"
					"\n"
					"\t\tp {\n"
					"\t\t\tcolor: #666;\n"
					"\t\t}\n"
					"\t</style>\n"
					"</head>\n"
					"\n"
					"<body>\n"
					"\t<div class=\"container\">\n"
					"\t\t<h1>" +
					title +
					"</h1>\n"
					"\t\t<a href=\"/\">Go back to root.</a>\n"
					"\t</div>\n"
					"</body>\n"
					"\n"
					"</html>";
		}
		_headers["content-type"] = "text/html";
	}

	void buildPage(RequestParsingResult& request) {
		std::string uri = findFinalUri(request.success.uri, _rootDir, request.location);
		if (!readContent(uri, _body)) {
			return buildErrorPage(request, STATUS_NOT_FOUND);
		}
		std::string extension = getExtension(uri);
		std::map<std::string, std::string>::const_iterator it = MIME_TYPES.find(extension);
		_headers["content-type"] = it != MIME_TYPES.end() ? it->second : DEFAULT_CONTENT_TYPE;
	}

	static void exportEnv(std::vector<std::string>& env, const std::string& key,
						  const std::string& value) {
		env.push_back(key + '=' + value);
	}

	static std::vector<std::string> createCgiEnv(const RequestParsingResult& request,
												 const std::string& finalUri) {

		std::vector<std::string> env;
		exportEnv(env, "CONTENT_LENGTH", toString(request.success.body.size()));
		std::map<std::string, std::string>::const_iterator it =
			request.success.headers.find("content-type");
		exportEnv(env, "CONTENT_TYPE",
				  it == request.success.headers.end() ? DEFAULT_CONTENT_TYPE : it->second);
		exportEnv(env, "DOCUMENT_ROOT", request.virtualServer->getRootDir());
		exportEnv(env, "GATEWAY_INTERFACE", CGI_VERSION);
		for (std::map<std::string, std::string>::const_iterator it =
				 request.success.headers.begin();
			 it != request.success.headers.end(); ++it) {
			if (CGI_NO_TRANSMISSION.find(it->first) == CGI_NO_TRANSMISSION.end()) {
				exportEnv(env, "HTTP_" + strupper(it->first), it->second);
			}
		}
		const std::string absolutePath = getAbsolutePath(finalUri);
		exportEnv(env, "PATH_INFO", absolutePath);
		exportEnv(env, "QUERY_STRING", request.success.query);
		exportEnv(env, "REDIRECT_STATUS", "200");
		exportEnv(env, "REQUEST_METHOD", toString(request.success.method));
		exportEnv(env, "SCRIPT_NAME", request.success.uri);
		exportEnv(env, "SCRIPT_FILENAME", absolutePath);
		exportEnv(env, "SERVER_PROTOCOL", HTTP_VERSION);
		exportEnv(env, "SERVER_SOFTWARE", SERVER_VERSION);
		return env;
	}

	static void cgiChild(RequestParsingResult& request, int childToParent[2], int parentToChild[2],
						 char* strExec, char* strScript, const std::string& finalUri) {
		close(parentToChild[1]);
		dup2(parentToChild[0], STDIN_FILENO);
		close(parentToChild[0]);
		close(childToParent[0]);
		dup2(childToParent[1], STDOUT_FILENO);
		close(childToParent[1]);
		char* argv[] = {strExec, strScript, NULL};
		char** env = vectorToCharArray(createCgiEnv(request, finalUri));
		execve(strExec, argv, env);
		perrored("execve");
		for (size_t i = 0; env[i]; ++i) {
			if (env[i]) {
				delete env[i];
			}
		}
		delete env;
		std::exit(EXIT_FAILURE);
	}

	void translateCgiResponse(RequestParsingResult& request, const std::string& response) {
		std::istringstream iss(response);
		std::string line;
		while (std::getline(iss, line) && !line.empty()) {
			size_t colon = line.find(':');
			if (colon != std::string::npos) {
				std::string key = strlower(strtrim(line.substr(0, colon), SPACES));
				std::string value = strtrim(line.substr(colon + 1), SPACES);
				if (key == "set-cookie") {
					_setCookies.push_back(value);
					continue;
				}
				_headers[key] = value;
				if (key == "status") {
					std::istringstream iss(value);
					iss >> value;
					if (value.size() != 3 || !std::isdigit(value[0]) || !std::isdigit(value[1]) ||
						!std::isdigit(value[2])) {
						_statusCode = STATUS_INTERNAL_SERVER_ERROR;
					} else {
						StatusCode castCode = static_cast<StatusCode>(std::atoi(value.c_str()));
						bool validStatusCode =
							STATUS_MESSAGES.find(castCode) != STATUS_MESSAGES.end();
						_statusCode = validStatusCode ? castCode : STATUS_INTERNAL_SERVER_ERROR;
					}
				}
			}
		}
		if (_statusCode < 200 || _statusCode > 299) {
			buildErrorPage(request, _statusCode);
		} else {
			std::stringstream buffer;
			buffer << iss.rdbuf();
			_body = buffer.str();
		}
	}

	void buildCgi(RequestParsingResult& request) {
		char* strExec = const_cast<char*>(_cgiExec.c_str());
		std::string finalUri = findFinalUri(request.success.uri, _rootDir, request.location);
		char* strScript = const_cast<char*>(finalUri.c_str());
		std::string body = std::string(request.success.body.begin(), request.success.body.end());
		if (access(strScript, F_OK) != 0) {
			return buildErrorPage(request, STATUS_BAD_GATEWAY);
		} else if (_autoIndex && isDirectory(strScript)) {
			return buildAutoIndexPage(request);
		} else if (body.size() > PIPE_SIZE) {
			return buildErrorPage(request, STATUS_PAYLOAD_TOO_LARGE);
		}

		int childToParent[2];
		int parentToChild[2];
		syscall(pipe(childToParent), "pipe");
		syscall(pipe(parentToChild), "pipe");
		pid_t pid = fork();
		syscall(pid, "fork");
		if (pid == 0) {
			cgiChild(request, childToParent, parentToChild, strExec, strScript, finalUri);
		}

		if (DEBUG) {
			std::cout << strExec << ' ' << strScript << " started with pid " << pid << ".\n";
		}
		_statusCode = STATUS_OK;
		close(parentToChild[0]);
		close(childToParent[1]);
		write(parentToChild[1], body.c_str(), body.size());
		close(parentToChild[1]);
		int exitCode = getExitCode(pid);
		if (DEBUG) {
			std::cout << strExec << ' ' << strScript << " exited with code " << exitCode << ".\n";
		}
		std::string response = fullRead(childToParent[0]);
		close(childToParent[0]);
		if (exitCode == 0) {
			translateCgiResponse(request, response);
		} else {
			buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
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
				std::string uri = request.success.uri == "/" ? "/" : request.success.uri + "/";
				filepath = findFinalUri(uri, _rootDir, request.location) + *it;
				if (isValidFile(filepath)) {
					std::string indexFile = (*it)[0] == '/' ? (*it).substr(1) : *it;
					request.success.uri = uri + indexFile;
					request.location =
						request.virtualServer->findMatchingLocation(request.success.uri);
					reinitResponseVariables(request);
					buildResponse(request);
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
		DIR* dir = opendir(findFinalUri(request.success.uri, _rootDir, request.location).c_str());
		if (!dir) {
			return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
		}
		_headers["content-type"] = "text/html";

		_body += "<!DOCTYPE html>\n";
		_body += "<html>\n";
		_body += "<head>\n";
		_body += "<title>" + request.success.uri + "</title>\n";
		_body += "<meta charset=\"utf-8\">\n";
		_body += "<style>\n";
		_body += "body { font-family: Arial, sans-serif; background-color: #f5f5f5; color: #333; "
				 "margin: 40px; }\n";
		_body +=
			"h1 { border-bottom: 1px solid #eee; padding-bottom: 0.3em; margin-bottom: 20px; }\n";
		_body += "li { margin: 10px 0; font-size: 1.2em; }\n";
		_body += "ul { list-style: none; padding-left: 0; }\n";
		_body += "a { color: #3498db; text-decoration: none; }\n";
		_body += "a:hover { color: #2980b9; }\n";
		_body += "div { width: 80%; margin: auto; max-width: 800px; }\n";
		_body += "</style>\n";
		_body += "</head>\n";
		_body += "<body>\n";
		_body += "<div>\n";
		_body += "<h1>Index of " + request.success.uri + "</h1>\n";
		_body += "<ul>\n";

		struct dirent* entry;
		while ((entry = readdir(dir))) {
			if (std::strcmp(entry->d_name, ".") == 0) {
				continue;
			}
			bool isParent = std::strcmp(entry->d_name, "..") == 0;
			if (isParent && request.success.uri == "/") {
				continue;
			}
			std::string name =
				isParent ? "↩" : std::string(entry->d_name) + (entry->d_type == DT_DIR ? "/" : "");
			std::string link =
				request.success.uri + (request.success.uri == "/" ? "" : "/") + entry->d_name;
			_body += "<li><a href=\"" + link + "\">" + name + "</a></li>\n";
		}
		closedir(dir);

		_body += "</ul>\n";
		_body += "</div>\n";
		_body += "</body>\n";
		_body += "</html>\n";
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
				_allowedMethods[i] = location->getAllowedMethods()[i];
			}
			_cgiExec = location->getCgiExec();
		}
	}
};