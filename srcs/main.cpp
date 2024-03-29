#include "../includes/webserv.hpp"

bool run = true;
const std::map<StatusCode, std::string> STATUS_MESSAGES;
const std::map<std::string, std::string> MIME_TYPES;
const std::set<std::string> CGI_NO_TRANSMISSION;

int main(int argc, char* argv[]) {
	std::signal(SIGINT, signalHandler);
	const char* conf = argc == 2 ? argv[1] : "conf/valid/everything.conf";
	if (argc > 2 || !endswith(conf, ".conf")) {
		std::cerr << "Usage: " << argv[0] << " [filename.conf]\n";
		return EXIT_FAILURE;
	}
	initGlobals();
	Server server;
	try {
		if (!server.init(conf)) {
			return EXIT_FAILURE;
		}
		std::cout << BLUE << "Press Ctrl+C to exit." << RESET << '\n';
		server.loop();
		std::cout << BLUE << "\rGood bye. 💞" << RESET << '\n';
		return EXIT_SUCCESS;
	} catch (const SystemError& e) {
		perrored(e.funcName);
		return EXIT_FAILURE;
	}
}