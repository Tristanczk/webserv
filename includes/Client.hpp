#pragma once

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
	~Client() {
		if (_currentRequest != NULL)
			delete _currentRequest;
	};

	ResponseStatusEnum handleRequests() {
		std::string request = readRequest();
		std::cout << "=== REQUEST START ===" << std::endl;
		std::cout << strtrim(request, "\r\n") << std::endl;
		std::cout << "=== REQUEST END ===" << std::endl;
		if (_currentRequest == NULL)
			_currentRequest = new Request(_associatedServers, _ip, _port);
		RequestParsingResult result = _currentRequest->parse(request.c_str(), request.size());
		if (result.result == REQUEST_PARSING_PROCESSING)
			return RESPONSE_PENDING;
		Response res =
			result.location
				? Response(result.location->getRootDir(), result.location->getAutoIndex(),
						   result.location->getErrorPages(), result.location->getIndexPages(),
						   result.location->getUri(), result.location->getReturn(),
						   result.location->getAllowedMethod(), result.location->getCgiExec(),
						   result.location->getCgiScript())
				: Response(result.virtualServer->getRootDir(), result.virtualServer->getAutoIndex(),
						   result.virtualServer->getErrorPages(),
						   result.virtualServer->getIndexPages());
		res.buildResponse(result);
		delete _currentRequest;
		_currentRequest = NULL;
		return res.pushResponseToClient(_fd) ? RESPONSE_SUCCESS : RESPONSE_FAILURE;
	}

	void setInfo(int fd) {
		_fd = fd;
		struct sockaddr_in localAddr;
		socklen_t localAddrLen = sizeof(localAddr);
		syscall(getsockname(_fd, (struct sockaddr*)&localAddr, &localAddrLen), "getsockname");
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

	std::string readRequest() { return fullRead(_fd); }

public:
	void printHostPort() {
		std::cout << "Client host:port: " << getIpString(_ip) << ":" << ntohs(_port) << std::endl;
	}
};