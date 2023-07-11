#pragma once

#include "webserv.hpp"
#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

extern const std::map<StatusCode, std::string> STATUS_MESSAGES;
extern const std::map<std::string, std::string> MIME_TYPES;

class Response {
public:
	// constructor in case of non matching location block, there won't be a locationUri, a redirect,
	// allowedMethods or link to cgiExec as they are exclusively defined in the location block
	Response(std::string rootDir, bool autoIndex, std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages)
		: _bodyPos(0), _rootDir(rootDir), _autoIndex(autoIndex), _serverErrorPages(errorPages),
		  _indexPages(indexPages), _return(-1, "") {
		initAllowedMethods(_allowedMethods);
		initMethodMap();
	}

	Response(std::string rootDir, std::string uploadDir, bool autoIndex,
			 std::map<int, std::string> const& serverErrorPages,
			 std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages, std::string locationUri,
			 std::pair<long, std::string> redirect, const bool allowedMethods[NO_METHOD],
			 std::string cgiExec)
		: _bodyPos(0), _rootDir(rootDir), _uploadDir(uploadDir), _autoIndex(autoIndex),
		  _serverErrorPages(serverErrorPages), _errorPages(errorPages), _indexPages(indexPages),
		  _locationUri(locationUri), _return(redirect), _cgiExec(cgiExec) {
		for (int i = 0; i < NO_METHOD; ++i)
			_allowedMethods[i] = allowedMethods[i];
		initMethodMap();
	}

	~Response(){};

	void buildResponse(RequestParsingResult& request) {
		if (request.result == REQUEST_PARSING_FAILURE) {
			_statusCode = request.statusCode;
			buildErrorPage(request);
		} else if (!_allowedMethods[request.success.method]) {
			_statusCode = STATUS_METHOD_NOT_ALLOWED;
			buildErrorPage(request);
		} else if (!_cgiExec.empty()) {
			buildCgi(request);
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
			std::cout << "=== RESPONSE START ===" << std::endl;
			if (!pushStringToClient(fd, _statusLine))
				return RESPONSE_FAILURE;
			for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
				 it != _headers.end(); it++) {
				line = it->first + ": " + it->second + "\r\n";
				if (!pushStringToClient(fd, line))
					return RESPONSE_FAILURE;
			}
			line = "\r\n";
			if (!pushStringToClient(fd, line))
				return RESPONSE_FAILURE;
		}
		if (!pushBodyChunkToClient(fd))
			return RESPONSE_FAILURE;
		std::cout << _bodyPos << " / " << _body.size() << std::endl;
		if (_bodyPos == _body.size()) {
			std::cout << "=== RESPONSE END ===" << std::endl;
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
	std::size_t _bodyPos;
	StatusCode _statusCode;

	// these variables will be extracted from the correct location or from the correct virtual
	// server if needed
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

	// function templates
	void buildGet(RequestParsingResult& request) {
		_statusCode = STATUS_OK;
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
			handleRedirect(request);
			return;
		}
		if (!buildPage(request))
			buildErrorPage(request);
	}

	void buildPost(RequestParsingResult& request) {
		std::cout << RED << "POST" << RESET << std::endl;
		std::map<std::string, std::string>::const_iterator it =
			request.success.headers.find("content-type");
		bool error = false;
		if (it == request.success.headers.end()) {
			_statusCode = STATUS_BAD_REQUEST;
			error = true;
		} else if (it->second == "application/x-www-form-urlencoded" ||
				   it->second == "multipart/form-data") {
			_statusCode = STATUS_UNSUPPORTED_MEDIA_TYPE;
			error = true;
		}
		if (error) {
			buildErrorPage(request);
			return;
		}
		if (!isDirectory(_uploadDir)) {
			_statusCode = STATUS_NOT_FOUND;
			buildErrorPage(request);
			return;
		}
		std::ofstream ofs(getFileUri(request).c_str());
		if (ofs.fail()) {
			_statusCode = STATUS_BAD_REQUEST;
			buildErrorPage(request);
			return;
		}
		std::string bodyStr(request.success.body.begin(), request.success.body.end());
		ofs << bodyStr;
		ofs.close();
		_statusCode = STATUS_CREATED;
	}

	void buildDelete(RequestParsingResult& request) {
		std::string uri = getFileUri(request);
		bool error = false;
		if (isDirectory(uri)) {
			_statusCode = STATUS_FORBIDDEN;
			error = true;
		} else if (!isValidFile(uri)) {
			_statusCode = STATUS_NOT_FOUND;
			error = true;
		} else {
			if (std::remove(uri.c_str()) == -1) {
				_statusCode = STATUS_FORBIDDEN;
				error = true;
			} else {
				_statusCode = STATUS_NO_CONTENT;
			}
		}
		if (error)
			buildErrorPage(request);
	}

	// TODO remove this function and use EPOLLOUT instead
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

	bool pushBodyChunkToClient(int fd) {
		std::cout << std::strlen(_body.c_str()) << std::endl;
		long cur_sent =
			send(fd, _body.c_str() + _bodyPos,
				 std::min(_body.size() - _bodyPos, static_cast<size_t>(RESPONSE_BUFFER_SIZE)),
				 MSG_NOSIGNAL);
		if (cur_sent < 0) {
			std::cerr << "Error: send failed here" << std::endl;
			return false;
		}
		std::cout << "sent " << cur_sent << " bytes" << std::endl;
		_bodyPos += cur_sent;
		return true;
	}

	void buildStatusLine() {
		_statusLine = std::string(HTTP_VERSION) + " " + toString(_statusCode) + " " +
					  STATUS_MESSAGES.find(_statusCode)->second + "\r\n";
	}

	// TODO Kebab-Case maybe?
	void buildHeader() {
		_headers["date"] = getDate();
		_headers["server"] = SERVER_VERSION;
		_headers["content-length"] = toString(_body.length());
		if (_headers.find("content-type") == _headers.end())
			_headers["content-type"] = DEFAULT_CONTENT_TYPE;
		//_headers["connection"] = "close";
	}

	void buildErrorPage(RequestParsingResult& request) {
		std::map<int, std::string>::iterator it = _errorPages.find(_statusCode);
		std::string errorPageUri;
		if (it != _errorPages.end())
			errorPageUri = "." + _rootDir + it->second;
		else {
			it = _serverErrorPages.find(_statusCode);
			errorPageUri = it != _errorPages.end()
							   ? "." + request.virtualServer->getRootDir() + it->second
							   : "./www/error/default_error.html";
		}
		std::cout << errorPageUri << std::endl;
		if (!readContent(errorPageUri, _body)) {
			// TODO return 500 Internal Server Error instead maybe
			_body = "There was an error while trying to access the specified error page for error "
					"code " +
					toString(_statusCode);
			_headers["content-type"] = "text/plain";
		} else
			_headers["content-type"] = "text/html";
	}

	bool buildPage(RequestParsingResult& request) {
		std::string uri = findFinalUri(request);
		std::cout << "final uri: " << uri << std::endl;
		if (!readContent(uri, _body)) {
			_statusCode = STATUS_NOT_FOUND;
			return false;
		}
		std::string extension = getExtension(uri);
		std::map<std::string, std::string>::const_iterator it = MIME_TYPES.find(extension);
		_headers["content-type"] = it != MIME_TYPES.end() ? it->second : DEFAULT_CONTENT_TYPE;
		return true;
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

	// TODO use EPOLLOUT to send the response
	void buildCgi(RequestParsingResult& request) {
		(void)request;
		_statusCode = STATUS_OK;
		std::cout << RED;
		std::cout << "locationUri " << _locationUri << std::endl;
		std::cout << "rootDir " << _rootDir << std::endl;
		std::cout << "finalUri " << findFinalUri(request) << std::endl;
		char* strExec = const_cast<char*>(_cgiExec.c_str());
		char* strScript =
			const_cast<char*>("./www/calculator/cgi-bin/calculator.py"); // TODO findFinalUri
		std::cout << RED << "CGI: " << strExec << ' ' << strScript << ' ' << request.success.query
				  << std::endl;
		std::cout << RESET;

		int pipefd[2];
		syscall(pipe(pipefd), "pipe");
		pid_t pid = fork();
		syscall(pid, "fork");
		if (pid == 0) {
			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[1]);
			char* argv[] = {strExec, strScript, NULL};
			char* env[CGI_ENV_SIZE];
			std::memset(env, 0, sizeof(env));
			createEnv(env, request);
			execve(strExec, argv, env);
			std::perror("execve");
			exit(EXIT_FAILURE);
		}
		close(pipefd[1]);
		std::string response = fullRead(pipefd[0]);
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
						isdigit(value[2]))
						_statusCode = static_cast<StatusCode>(std::atoi(value.c_str()));
					else {
						_statusCode = STATUS_INTERNAL_SERVER_ERROR;
						buildErrorPage(request);
						return;
					}
				}
			}
		}
		std::stringstream buffer;
		buffer << iss.rdbuf();
		_body = buffer.str();
		close(pipefd[0]);
		int wstatus;
		wait(&wstatus);
		const int exitCode = WIFSIGNALED(wstatus) ? 128 + WTERMSIG(wstatus) : WEXITSTATUS(wstatus);
		if (exitCode == 0) {
			_statusCode = STATUS_OK;
		} else {
			std::cerr << RED << strExec << " " << strScript << " failed with exit code " << exitCode
					  << RESET << std::endl;
			_statusCode = STATUS_INTERNAL_SERVER_ERROR;
		}
	}

	std::string findFinalUri(RequestParsingResult& request) {
		// note : this function will be called after checking if the requested uri is a file or
		// a directory
		Location* location = request.location;
		std::string uri = request.success.uri;
		// to ensure that the final link will be well formated whether the user put a trailing
		// slash at the end of the location and at the beginning of the uri or not
		if (_rootDir[_rootDir.size() - 1] == '/')
			_rootDir = _rootDir.substr(0, _rootDir.size() - 1);
		if (uri[0] == '/')
			uri = uri.substr(1);
		if (location == NULL)
			return "." + _rootDir + "/" + uri;
		LocationModifierEnum modifier = location->getModifier();
		if (modifier == DIRECTORY)
			return "." + _rootDir + "/" + uri.substr(_locationUri.size() - 1);
		else if (modifier == REGEX) {
			// in case of a matching regex, we append the whole path to the root directory
			return "." + _rootDir + "/" + uri;
		} else {
			// in case of an exact match, we append only the file name to the root directory
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
				std::cout << "filepath: " << filepath << std::endl;
				if (isValidFile(filepath)) {
					std::cout << "valid index file: " << *it << std::endl;
					std::string indexFile = (*it)[0] == '/' ? (*it).substr(1) : *it;
					request.success.uri += indexFile;
					request.location =
						request.virtualServer->findMatchingLocation(request.success.uri);
					reinitResponseVariables(request);
					if (!buildPage(request))
						buildErrorPage(request);
					return;
				}
				std::cout << "invalid index file: " << *it << std::endl;
			}
		}
		if (_autoIndex) {
			_statusCode = STATUS_OK;
			buildAutoIndexPage(request);
		} else {
			_statusCode = STATUS_FORBIDDEN;
			buildErrorPage(request);
		}
	}

	void handleRedirect(RequestParsingResult& request) {
		_statusCode = static_cast<StatusCode>(_return.first);
		buildErrorPage(request);
		_headers["location"] = _return.second;
	}

	void buildAutoIndexPage(RequestParsingResult& request) {
		DIR* dir = opendir(findFinalUri(request).c_str());
		if (dir == NULL) {
			_statusCode = STATUS_INTERNAL_SERVER_ERROR;
			buildErrorPage(request);
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
			for (int i = 0; i < NO_METHOD; i++)
				_allowedMethods[i] = true;
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
			for (int i = 0; i < NO_METHOD; i++)
				_allowedMethods[i] = location->getAllowedMethod()[i];
			_cgiExec = location->getCgiExec();
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
	//  	- status code (if error when parsing request, use that error code, otherwise check
	//  if the request can be answered, if ok code will be 200)
	//  	- status message (what do we put here?)
	//  2. headers: what are the field we need ?
	//  	- Content-Length (if body is empty, set to 0)
	//  	- Content-Type (if body is empty, set to text/html ?)
	//  	- Date (use current date)
	//  	- Server (use server name)
	//  	- Connection : at first we manade only connection: close, we will see if we handle
	//  the keep-alive mode later
	//  3. body:
	//		- if there is an error code, we need to build a body with the error message based on
	// the
	// correct error page, specific error page or default error page
	//  	- content of the html pages
	//  	- if it is a php page, do we need to convert it to html before sending it ? I think
	//  yes but not sure
	//		- handle differently whether the uri is a directory or a file
	//		- if it is a directory, we need to handle it according to the index and autoindex
	// options
};