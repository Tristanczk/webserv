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

	std::string readRequest() {
		std::size_t bufferSize = _associatedServer->getBufferSize();
		return fullRead(_fd, bufferSize);
	}

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

	struct sockaddr_in& getAddress() { return _address; }
	socklen_t& getAddressLen() { return _addressLen; }
	VirtualServer* getAssociatedServer() { return _associatedServer; }
	in_addr_t getIp() { return _ip; }
	in_port_t getPort() { return _port; }

private:
	VirtualServer* _associatedServer;
	struct sockaddr_in _address;
	socklen_t _addressLen;
	in_addr_t _ip;
	in_port_t _port;
	int _fd;
};