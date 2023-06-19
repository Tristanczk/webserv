#pragma once

#include "webserv.hpp"
#include <cstddef>
#include <vector>

class Client {
public:
	Client() : _currentMatchingServer(NULL), _addressLen(sizeof(_address)) {
		std::memset(&_address, 0, sizeof(_address));
	};
	~Client(){};

	std::string readRequest() {
		std::size_t bufferSize = _currentMatchingServer->getBufferSize();
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

	struct sockaddr_in& getAddress() { return _address; }
	socklen_t& getAddressLen() { return _addressLen; }
	in_addr_t getIp() { return _ip; }
	in_port_t getPort() { return _port; }

private:
	std::vector<VirtualServer*> _associatedServers;
	VirtualServer* _currentMatchingServer;
	struct sockaddr_in _address;
	socklen_t _addressLen;
	in_addr_t _ip;
	in_port_t _port;
	int _fd;
};