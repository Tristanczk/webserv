#pragma once

#include "webserv.hpp"

class Server {
public:
	Server() {};
	~Server() {};

	bool parseConfig(const std::string& file) {
		std::ifstream config(file.c_str());
		if (!config.good()) {
			std::cerr << "Cannot open file" << std::endl;
			return false;
		}
		for (std::string line; std::getline(config, line);) {
			if (line[0] == '#' || line.empty())
				continue;
			if (line == "server {") {
				VirtualServer server;
				if (!server.initServer(config))
					return false;
				_virtualServers.push_back(server);
			} else {
				std::cerr << "Invalid line in config file: " << line << std::endl;
				return false;
			}
		}
		if (!checkInvalidServers())
			return false;
		return true;
	}

	void printVirtualServerList() const {
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			std::cout << "Server " << i << ":" << std::endl;
			_virtualServers[i].printServerInformation();
		}
	}

private:
	std::vector<VirtualServer> _virtualServers;
	
	bool checkInvalidServers() const {
		for (size_t i = 0; i < _virtualServers.size(); ++i) {
			for (size_t j = 0; j < _virtualServers.size(); ++j) {
				if (i == j)
					continue;
				if (_virtualServers[i].getPort() == _virtualServers[j].getPort() && _virtualServers[i].getAddr() == _virtualServers[j].getAddr()) {
					std::vector<std::string> serverNamesI = _virtualServers[i].getServerNames();
					std::vector<std::string> serverNamesJ = _virtualServers[j].getServerNames();
					if (serverNamesI.empty() && serverNamesJ.empty())
					{
						std::cerr << "Conflicting server on host:port " << getIpString(_virtualServers[i].getAddr()) << ":" << ntohs(_virtualServers[i].getPort())
						<< " for server name \"\"" << std::endl;
						return false;
					}	
					for (size_t k = 0; k < serverNamesI.size(); ++k) {
						for (size_t l = 0; l < serverNamesJ.size(); ++l) {
							if (serverNamesI[k] == serverNamesJ[l]) {
								std::cerr << "Conflicting server on host:port " << getIpString(_virtualServers[i].getAddr()) << ":" << ntohs(_virtualServers[i].getPort())
								<< " for server name: " << serverNamesI[k] << std::endl;
								return false;
							}
						}
					}
				}
			}
		}
		return true;
	}
};