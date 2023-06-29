#pragma once

#include "Request.hpp"
#include "webserv.hpp"

class Client {
public:
	Client()
		: _currentMatchingServer(NULL), _currentMatchingLocation(NULL),
		  _addressLen(sizeof(_address)), _currentRequest(NULL) {
		std::memset(&_address, 0, sizeof(_address));
		(void)_currentMatchingServer;
		(void)_currentMatchingLocation;
	};
	~Client(){};

	std::string readRequest() { return fullRead(_fd); }

	void handleRequests() {
		std::string request = readRequest();
		if (_currentRequest == NULL)
			_currentRequest = new Request(_associatedServers, _ip, _port);
		RequestParsingResult result = _currentRequest->parse(request.c_str(), request.size());
		if (result.result == REQUEST_PARSING_PROCESSING)
			return;
		VirtualServer* vs = result.virtualServer;
		Location* loc = result.location;
		Response res(vs->getRootDir(), vs->getAutoIndex(), vs->getErrorPages(),
					 vs->getIndexPages());
		if (loc != NULL)
			res = Response(loc->getRootDir(), loc->getAutoIndex(), loc->getErrorPages(),
						   loc->getIndexPages(), loc->getUri(), loc->getReturn(),
						   loc->getAllowedMethod(), loc->getCgiExec());
		(void)res;
		// TODO: parse header
		// get the server name in the host part
		// find the best matching server (using the findBestMatch method)
		// get the value for the max body size
		return;
	}

	void setInfo(int fd) {
		_fd = fd;
		struct sockaddr_in localAddr;
		socklen_t localAddrLen = sizeof(localAddr);
		syscall(getsockname(fd, (struct sockaddr*)&localAddr, &localAddrLen), "getsockname");
		_ip = localAddr.sin_addr.s_addr;
		_port = localAddr.sin_port;
	}

	void findAssociatedServers(std::vector<VirtualServer>& vs) {
		for (size_t i = 0; i < vs.size(); ++i) {
			if (vs[i].getPort() == _port &&
				(vs[i].getAddr() == _ip || vs[i].getAddr() == INADDR_ANY)) {
				_associatedServers.push_back(&vs[i]);
			}
		}
	}

	std::string buildResponse(char* const envp[], std::string& filename) {
		std::string cgiExec = _currentMatchingLocation->getCgiExec();
		if (cgiExec.empty()) {
			// TODO
			//  return buildResponseHTML();
			return "";
		} else {
			// build the header
			std::string body = buildCgiBody(cgiExec, filename, envp);
			// TODO
			return "";
		}
	}

	std::string buildCgiBody(std::string path_to_exec, std::string filename, char* const envp[]) {
		std::string finalPath;
		if (!getValidPath(path_to_exec, envp, finalPath))
			// do we throw an exception in this case or do we handle the error differently?
			// TODO return internal server error probably
			throw SystemError("Invalid path for CGI");
		int pipefd[2];
		syscall(pipe(pipefd), "pipe");
		pid_t pid = fork();
		syscall(pid, "pipe");
		if (pid == 0) {
			char* const argv[] = {const_cast<char*>(finalPath.c_str()),
								  const_cast<char*>(filename.c_str()), NULL};
			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[1]);
			execve(finalPath.c_str(), argv, envp);
			exit(EXIT_FAILURE);
		}
		// TODO wait and return INTERNAL SERVER ERROR if status != EXIT_SUCCESS
		close(pipefd[1]);
		std::string response = fullRead(pipefd[0]);
		close(pipefd[0]);
		return response;
	}

	struct sockaddr_in& getAddress() { return _address; }
	socklen_t& getAddressLen() { return _addressLen; }
	in_addr_t getIp() { return _ip; }
	in_port_t getPort() { return _port; }

private:
	std::vector<VirtualServer*> _associatedServers;
	VirtualServer* _currentMatchingServer;
	Location* _currentMatchingLocation;
	struct sockaddr_in _address;
	socklen_t _addressLen;
	in_addr_t _ip;
	in_port_t _port;
	int _fd;
	std::queue<Response> _responseQueue;
	Request* _currentRequest;

public:
	void printHostPort() {
		std::cout << "Client host:port: " << getIpString(_ip) << ":" << ntohs(_port) << std::endl;
	}
};