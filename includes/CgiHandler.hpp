#pragma once

#include "webserv.hpp"

extern const std::set<std::string> CGI_NO_TRANSMISSION;

class CgiHandler {
public:
	CgiHandler(const std::string& cgiExec, const RequestParsingResult& request,
			   const std::string& finalUri)
		: _cgiExec(cgiExec), _request(request), _pid(INT_MAX), _finalUri(finalUri) {}

	~CgiHandler() {
		if (_pid != INT_MAX) {
			kill(_pid, SIGTERM);
		}
		// TODO close sockets
	}

	StatusCode init() {
		char* strExec = const_cast<char*>(_cgiExec.c_str());
		char* strScript = const_cast<char*>(_finalUri.c_str());
		if (access(strScript, F_OK) != 0) {
			return STATUS_NOT_FOUND;
		}
		int childToParent[2];
		int parentToChild[2];
		syscall(pipe(childToParent), "pipe");
		syscall(pipe(parentToChild), "pipe");
		pid_t pid = fork();
		syscall(pid, "fork");
		if (pid == 0) {
			cgiChild(childToParent, parentToChild, strExec, strScript);
		}
		_pid = pid;

		std::string _body = std::string(_request.success.body.begin(), _request.success.body.end());
		close(parentToChild[0]);
		close(childToParent[1]);
		syscallEpoll(parentToChild[1], EPOLL_CTL_ADD, clientFd, EPOLLIN | EPOLLRDHUP,
					 "EPOLL_CTL_ADD");

		write(parentToChild[1], body.c_str(), body.size());
		close(parentToChild[1]);
		std::string response = fullRead(childToParent[0]);
		close(childToParent[0]);
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
					if (value.size() == 3 && std::isdigit(value[0]) && std::isdigit(value[1]) &&
						std::isdigit(value[2])) {
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
			std::cout << RED << strExec << ' ' << strScript << " failed with code " << exitCode
					  << '.' << std::endl;
			return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
		}

		return STATUS_NONE;
	}

private:
	pid_t _pid;
	std::string _body;
	std::string _finalUri;
	std::string _cgiExec;
	RequestParsingResult _request;

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
};

// void buildCgi(RequestParsingResult& request) {
// 	char* strExec = const_cast<char*>(_cgiExec.c_str());
// 	std::string finalUri = findFinalUri(request);
// 	char* strScript = const_cast<char*>(finalUri.c_str());
// 	if (access(strScript, F_OK) != 0) {
// 		return buildErrorPage(request, STATUS_NOT_FOUND);
// 	} else if (_autoIndex && isDirectory(strScript)) {
// 		return buildAutoIndexPage(request);
// 	}
// 	int childToParent[2];
// 	syscall(pipe(childToParent), "pipe");
// 	int parentToChild[2];
// 	syscall(pipe(parentToChild), "pipe");
// 	pid_t pid = fork();
// 	syscall(pid, "fork");
// 	if (pid == 0) {
// 		cgiChild(request, childToParent, parentToChild, strExec, strScript, finalUri);
// 	}
// 	std::cout << strExec << ' ' << strScript << " started with pid " << pid << ".\n";
// 	_statusCode = STATUS_OK;
// 	close(parentToChild[0]);
// 	close(childToParent[1]);
// 	std::string body = std::string(request.success.body.begin(), request.success.body.end());
// 	write(parentToChild[1], body.c_str(), body.size());
// 	close(parentToChild[1]);
// 	std::string response = fullRead(childToParent[0]);
// 	close(childToParent[0]);
// 	int wstatus;
// 	wait(&wstatus);
// 	std::istringstream iss(response);
// 	std::string line;
// 	while (std::getline(iss, line) && !line.empty()) {
// 		size_t colon = line.find(':');
// 		if (colon != std::string::npos) {
// 			std::string key = strlower(strtrim(line.substr(0, colon), SPACES));
// 			std::string value = strtrim(line.substr(colon + 1), SPACES);
// 			_headers[key] = value;
// 			if (key == "status") {
// 				std::istringstream iss(value);
// 				iss >> value;
// 				if (value.size() == 3 && std::isdigit(value[0]) && std::isdigit(value[1]) &&
// 					std::isdigit(value[2])) {
// 					_statusCode = static_cast<StatusCode>(std::atoi(value.c_str()));
// 				} else {
// 					return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
// 				}
// 			}
// 		}
// 	}
// 	std::stringstream buffer;
// 	buffer << iss.rdbuf();
// 	_body = buffer.str();
// 	const int exitCode = WIFSIGNALED(wstatus) ? 128 + WTERMSIG(wstatus) : WEXITSTATUS(wstatus);
// 	if (exitCode != 0) {
// 		std::cout << RED << strExec << ' ' << strScript << " failed with code " << exitCode << '.'
// 				  << std::endl;
// 		return buildErrorPage(request, STATUS_INTERNAL_SERVER_ERROR);
// 	}
// }