#include "../includes/webserv.hpp"

bool run = true;
std::set<pid_t> pids;
const std::map<StatusCode, std::string> STATUS_MESSAGES;
const std::map<std::string, std::string> MIME_TYPES;
const std::set<std::string> CGI_NO_TRANSMISSION;

int main(int argc, char* argv[]) {
	std::signal(SIGINT, signalHandler);
	std::signal(SIGCHLD, SIG_IGN);
	const char* conf = argc == 2 ? argv[1] : "conf/valid/default.conf";
	if (argc > 2 || !endswith(conf, ".conf")) {
		std::cerr << "Usage: " << argv[0] << " [filename.conf]" << std::endl;
		return EXIT_FAILURE;
	}
	initGlobals();
	Server server;
	try {
		if (!server.init(conf)) {
			return EXIT_FAILURE;
		}
		std::cout << BLUE << getBasename(argv[0]) << " is running." << RESET << std::endl;
		std::cout << BLUE << "Press Ctrl+C to exit." << RESET << std::endl;
		server.loop();
		std::cout << BLUE << "\rGood bye. 💞" << RESET << std::endl;
		killChildren();
		return EXIT_SUCCESS;
	} catch (const SystemError& e) {
		perrored(e.funcName);
		killChildren();
		return EXIT_FAILURE;
	}
}