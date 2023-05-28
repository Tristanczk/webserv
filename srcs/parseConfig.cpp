#include "../includes/VirtualServer.hpp"
#include "../includes/webserv.hpp"
#include <vector>

bool parseConfig(std::string& file, std::vector<VirtualServer>& servers) {
	std::ifstream config(file);
	if (!config.good()) {
		std::cerr << "Cannot open file" << std::endl;
		return false;
	}
	for (std::string line; std::getline(config, line);) {
		if (line == "server {") {
			VirtualServer server;
			server.initServer(config);
			servers.push_back(server);
		} else {
			std::cerr << "Invalid line in config file: " << line << std::endl;
			return false;
		}
	}
	return true;
}