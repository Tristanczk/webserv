#include "../includes/webserv.hpp"

bool run = true;

int main(int argc, char* argv[]) {
	std::signal(SIGINT, signalHandler);
	const char* conf = argc == 2 ? argv[1] : "conf/valid/default.conf";
	if (argc > 2 || !endswith(conf, ".conf")) {
		std::cerr << "Usage: " << argv[0] << " [filename.conf]" << std::endl;
		return EXIT_FAILURE;
	}
	initStatusMessageMap();
	initMimeTypes();
	Server server;
	if (!server.init(conf))
		return EXIT_FAILURE;
	try {
		std::cout << BLUE << getBasename(argv[0]) << " is running." << RESET << std::endl;
		std::cout << BLUE << "Press Ctrl+C to exit." << RESET << std::endl;
		server.loop();
		std::cout << BLUE << "\rGood bye. 💞" << RESET << std::endl;
		return EXIT_SUCCESS;
	} catch (const SystemError& e) {
		std::perror(e.funcName);
		return EXIT_FAILURE;
	}
}