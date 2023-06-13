#pragma once

#include "webserv.hpp"
#include <netinet/in.h>

class Client {
public:
	Client() : _associatedServer(NULL), _addressLen(sizeof(_address)) {
		std::memset(&_address, 0, sizeof(_address));
	};
	Client(VirtualServer* vs) : _associatedServer(vs), _addressLen(sizeof(_address)) {
		std::memset(&_address, 0, sizeof(_address));
	};
	~Client(){};

	struct sockaddr_in& getAddress() { return _address; }
	socklen_t& getAddressLen() { return _addressLen; }
	VirtualServer* getAssociatedServer() { return _associatedServer; }
	void setFd(int fd) { _fd = fd; }

private:
	VirtualServer* _associatedServer;
	struct sockaddr_in _address;
	socklen_t _addressLen;
	int _fd;
};