#include "../includes/webserv.hpp"

bool parseConfig(std::string& file, std::vector<VirtualServer>& servers) {
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
	return true;
}