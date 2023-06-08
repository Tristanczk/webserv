#include "../includes/webserv.hpp"

bool parseConfig(const std::string& file, std::vector<VirtualServer>& servers) {
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
			servers.push_back(server);
		} else {
			std::cerr << "Invalid line in config file: " << line << std::endl;
			return false;
		}
	}
	if (!checkInvalidServers(servers))
		return false;
	return true;
}

bool checkInvalidServers(const std::vector<VirtualServer>& servers) {
	for (size_t i = 0; i < servers.size(); ++i) {
		for (size_t j = 0; j < servers.size(); ++j) {
			if (i == j)
				continue;
			if (servers[i].getPort() == servers[j].getPort() && servers[i].getAddr() == servers[j].getAddr()) {
				std::vector<std::string> serverNamesI = servers[i].getServerNames();
				std::vector<std::string> serverNamesJ = servers[j].getServerNames();
				if (serverNamesI.empty() && serverNamesJ.empty())
				{
					std::cerr << "Conflicting servers on host:port " << getIpString(servers[i].getAddr()) << ":" << ntohs(servers[i].getPort())
					<< " for server name \"\"" << std::endl;
					return false;
				}	
				for (size_t k = 0; k < serverNamesI.size(); ++k) {
					for (size_t l = 0; l < serverNamesJ.size(); ++l) {
						if (serverNamesI[k] == serverNamesJ[l]) {
							std::cerr << "Conflicting servers on host:port " << getIpString(servers[i].getAddr()) << ":" << ntohs(servers[i].getPort())
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