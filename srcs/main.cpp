#include "../includes/webserv.hpp"

int main(int argc, char* argv[]) {
	if (argc > 2 || (argc == 2 && !endswith(argv[1], ".conf"))) {
		std::cerr << "Usage: " << argv[0] << " [filename.conf]" << std::endl;
		return EXIT_FAILURE;
	}
	Server server;
	if (!server.parseConfig(argc == 2 ? argv[1] : DEFAULT_CONF))
		return EXIT_FAILURE;
	if (!server.initServer())
		return EXIT_FAILURE;
	server.loop();
	return EXIT_SUCCESS;
}