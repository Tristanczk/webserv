#pragma once

#include "webserv.hpp"

class Client {
public:
	Client() : _addressLen(sizeof(_address)), _currentRequest(NULL), _currentResponse(NULL) {
		std::memset(&_address, 0, sizeof(_address));
	};

	~Client() {
		if (_currentRequest != NULL)
			delete _currentRequest;
		if (_currentResponse != NULL)
			delete _currentResponse;
	};

	ResponseStatusEnum handleRequest() {
		std::string request = fullRead(_fd);
		if (request.empty())
			return RESPONSE_FAILURE;
		std::cout << YELLOW << "=== REQUEST START ===" << std::endl
				  << strtrim(request, "\r\n") << std::endl
				  << "=== REQUEST END ===" << std::endl
				  << RESET;
		if (_currentRequest == NULL)
			_currentRequest = new Request(_associatedServers, _ip, _port);
		RequestParsingResult result = _currentRequest->parse(request.c_str(), request.size());
		if (result.result == REQUEST_PARSING_PROCESSING)
			return RESPONSE_PENDING;
		_currentResponse =
			result.location
				? new Response(result.location->getRootDir(), result.location->getUploadDir(),
							   result.location->getAutoIndex(),
							   result.virtualServer->getErrorPages(),
							   result.location->getErrorPages(), result.location->getIndexPages(),
							   result.location->getUri(), result.location->getReturn(),
							   result.location->getAllowedMethod(), result.location->getCgiExec())
				: new Response(
					  result.virtualServer->getRootDir(), result.virtualServer->getAutoIndex(),
					  result.virtualServer->getErrorPages(), result.virtualServer->getIndexPages());
		_currentResponse->buildResponse(result);
		delete _currentRequest;
		_currentRequest = NULL;
		return RESPONSE_SUCCESS;
	}

	ResponseStatusEnum pushResponse() {
		ResponseStatusEnum status = _currentResponse->pushResponseToClient(_fd);
		// TODO check why it used to be: if (status == RESPONSE_SUCCESS)
		if (status != RESPONSE_PENDING) {
			delete _currentResponse;
			_currentResponse = NULL;
		}
		return status;
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
	struct sockaddr_in _address;
	socklen_t _addressLen;
	in_addr_t _ip;
	in_port_t _port;
	int _fd;
	std::queue<Response> _responseQueue;
	Request* _currentRequest;
	Response* _currentResponse;
};