#pragma once

#include "webserv.hpp"

class Client {
public:
	Client() : _currentMatchingServer(NULL), _addressLen(sizeof(_address)) {
		std::memset(&_address, 0, sizeof(_address));
	};
	~Client(){};

	std::string readRequest() {
		std::size_t bufferSize = BUFFER_SIZE;
		return fullRead(_fd, bufferSize);
	}

	// void handleRequests() {
	// 	std::string request = readRequest();
	// 	// TODO: parse header
	// 	// get the server name in the host part
	// 	// find the best matching server (using the findBestMatch method)
	// 	// get the value for the max body size
	// 	return true;
	// }

	void printHostPort() {
		std::cout << "Client host:port: " << getIpString(_ip) << ":" << ntohs(_port) << std::endl;
	}

	bool setInfo(int fd) {
		_fd = fd;
		struct sockaddr_in localAddr;
		socklen_t localAddrLen = sizeof(localAddr);
		if (getsockname(fd, (struct sockaddr*)&localAddr, &localAddrLen) == -1) {
			std::cerr << "Error while getting local address" << std::endl;
			return false;
		}
		_ip = localAddr.sin_addr.s_addr;
		_port = localAddr.sin_port;
		return true;
	}

	void findAssociatedServers(std::vector<VirtualServer>& vs) {
		for (size_t i = 0; i < vs.size(); ++i) {
			if (vs[i].getPort() == _port &&
				(vs[i].getAddr() == _ip || vs[i].getAddr() == INADDR_ANY)) {
				_associatedServers.push_back(&vs[i]);
			}
		}
	}

	VirtualServer* findBestMatch(const std::string& serverName) {
		int bestMatch = -1;
		t_vsmatch bestMatchLevel = VS_MATCH_NONE;
		for (size_t i = 0; i < _associatedServers.size(); ++i) {
			t_vsmatch matchLevel = _associatedServers[i]->isMatching(_port, _ip, serverName);
			if (matchLevel > bestMatchLevel) {
				bestMatch = i;
				bestMatchLevel = matchLevel;
			}
		}
		_currentMatchingServer = bestMatch == -1 ? NULL : _associatedServers[bestMatch];
		return _currentMatchingServer;
	}

	Location* findMatchingLocation(const std::string& uri) {
		if (_currentMatchingServer == NULL)
			return NULL;
		_currentMatchingLocation = _currentMatchingServer->findMatchingLocation(uri);
		return _currentMatchingLocation;
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
		int pipefd[2];
		std::string finalPath;
		if (!getValidPath(path_to_exec, envp, finalPath))
			// do we throw an exception in this case or do we handle the error differently?
			// TODO return internal server error probably
			throw SystemError("Invalid path for CGI");
		syscall(pipe(pipefd), "pipe");
		pid_t pid = fork();
		if (pid == -1)
			throw SystemError("fork");
		if (pid == 0) {
			char* const argv[] = {const_cast<char*>(finalPath.c_str()),
								  const_cast<char*>(filename.c_str()), NULL};
			close(pipefd[0]);
			if (dup2(pipefd[1], STDOUT_FILENO) == -1)
				throw SystemError("dup2");
			close(pipefd[1]);
			if (execve(finalPath.c_str(), argv, envp) == -1)
				throw SystemError("execve");
		}
		close(pipefd[1]);
		std::string response = fullRead(pipefd[0], BUFFER_SIZE);
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
};