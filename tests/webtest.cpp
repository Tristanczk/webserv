#include "webtest.hpp"

int status = EXIT_SUCCESS;
bool run = true;

void displayTitle(const std::string& title) {
	std::cout << BOLDBLUE << "--- " << title << " ---\n" << RESET;
}

void displayResult(const std::string& testName, bool result) {
	if (!result) {
		status = EXIT_FAILURE;
	}
	std::cout << (result ? GREEN : RED) << testName << ": " << (result ? "OK" : "KO") << RESET
			  << std::endl;
}

int main() {
	testParseConfig();
	testServer();
	testLocation();
	testFinalUri();
	return status;
}
