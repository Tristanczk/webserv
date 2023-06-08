#include "webtest.hpp"

static std::vector<std::string> ls(const std::string& path) {
	DIR* dp;
	struct dirent* ep;
	std::vector<std::string> filenames;

	dp = opendir(path.c_str());
	while ((ep = readdir(dp)))
		if (ep->d_name[0] != '.')
			filenames.push_back(path + ep->d_name);
	closedir(dp);
	std::sort(filenames.begin(), filenames.end());
	return filenames;
}

static void test(const std::string& title, const std::string& directory, bool expected) {
	displayTitle(title);
	std::vector<std::string> filenames = ls(directory);
	if (!expected)
		filenames.push_back(CONF_NOT_FOUND);
	for (std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end();
		 it++) {
		const std::string& filename = *it;
		const bool isUnreadable = filename == CONF_UNREADABLE;
		if (isUnreadable)
			chmod(CONF_UNREADABLE, 0000);
		Server server;
		bool result = server.parseConfig(filename);
		if (isUnreadable)
			chmod(CONF_UNREADABLE, 0644);
		displayResult(filename, result == expected);
	}
}

void testParseConfig() {
	test("VALID CONFS", VALID_DIRECTORY, true);
	test("INVALID CONFS", INVALID_DIRECTORY, false);
}
