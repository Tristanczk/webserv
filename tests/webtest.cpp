#include "webtest.hpp"

bool run = true;
int epollFd = -1;
std::set<pid_t> pids;
const std::map<StatusCode, std::string> STATUS_MESSAGES;
const std::map<std::string, std::string> MIME_TYPES;
const std::set<std::string> CGI_NO_TRANSMISSION;

int status = EXIT_SUCCESS;

void displayTitle(const std::string& title) {
	std::cout << BOLDBLUE << "--- " << title << " ---\n" << RESET;
}

void displayResult(const std::string& testName, bool result) {
	if (!result) {
		status = EXIT_FAILURE;
	}
	std::cout << (result ? GREEN : RED) << testName << ": " << (result ? "OK" : "KO") << RESET
			  << '\n';
}

int main() {
	testParseConfig();
	testServer();
	testLocation();
	testFinalUri();
	return status;
}
