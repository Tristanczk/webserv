#include "webtest.hpp"

// TODO call the real functions

static int findBestMatchServer(const std::string& ip, size_t port, const std::string& serverName,
							   std::vector<VirtualServer>& vServers) {
	int bestMatch = -1;
	in_addr_t ipValue;
	if (!getIpValue(ip, ipValue)) {
		return -2;
	}
	in_port_t portValue = htons(port);
	VirtualServerMatch bestMatchLevel = VS_MATCH_NONE;
	for (size_t i = 0; i < vServers.size(); ++i) {
		VirtualServerMatch matchLevel = vServers[i].isMatching(portValue, ipValue, serverName);
		if (matchLevel > bestMatchLevel) {
			bestMatch = i;
			bestMatchLevel = matchLevel;
		}
	}
	return bestMatch;
}

static int findBestMatchLocation(const std::string& uri, VirtualServer& server) {
	std::vector<Location> const& locations = server.getLocations();
	if (locations.empty() || uri.empty()) {
		return -1;
	}
	int curRegex = -1;
	int curPrefix = -1;
	int prefixLength = 0;
	for (size_t i = 0; i < locations.size(); ++i) {
		int matchLevel;
		try {
			matchLevel = locations[i].isMatching(uri);
		} catch (const RegexError& e) {
			std::cerr << e.what() << '\n';
			return -2;
		}
		if (matchLevel == LOCATION_MATCH_EXACT) {
			return i;
		} else if (matchLevel == LOCATION_MATCH_REGEX && curRegex == -1) {
			curRegex = i;
		} else if (matchLevel > prefixLength) {
			curPrefix = i;
			prefixLength = matchLevel;
		}
	}
	if (curRegex != -1) {
		return curRegex;
	} else if (curPrefix != -1) {
		return curPrefix;
	} else {
		return -1;
	}
}

void testServer() {
	Server server;
	server.parseConfig("./conf/valid/testmatching.conf");
	std::vector<VirtualServer>& vServers = server.getVirtualServers();
	displayTitle("MATCHING SERVER");
	std::ifstream config("./tests/testsmatchingserver.txt");
	if (!config.good()) {
		std::cerr << "Cannot open file for testing matching server\n";
		return;
	}
	int i = 0;
	for (std::string line; std::getline(config, line);) {
		if (line[0] == '#' || line.empty()) {
			continue;
		}
		std::istringstream iss(line);
		std::string ip, serverName;
		size_t port;
		int expected;
		if (!(iss >> ip >> port >> serverName >> expected)) {
			std::cerr << "Invalid line in testmatchingserver.txt: " << line << '\n';
			continue;
		}
		if (serverName == "*") {
			serverName = "";
		}
		std::string testNb = "Test " + toString(++i);
		int match = findBestMatchServer(ip, port, serverName, vServers);
		bool result = expected == match;
		displayResult(testNb, result);
		if (!result) {
			std::cerr << "Expected: " << expected << " | found: " << match << '\n';
		}
	}
}

void testLocation() {
	Server server;
	server.parseConfig("./conf/valid/testmatching.conf");
	VirtualServer& vServer = server.getVirtualServers()[0];
	displayTitle("MATCHING LOCATION");
	std::ifstream config("./tests/testsmatchinglocation.txt");
	if (!config.good()) {
		std::cerr << "Cannot open file for testing matching location\n";
		return;
	}
	for (std::string line; std::getline(config, line);) {
		if (line[0] == '#' || line.empty()) {
			continue;
		}
		std::istringstream iss(line);
		std::string uri;
		int expected;
		if (!(iss >> uri >> expected)) {
			std::cerr << RED << "Invalid line in testmatchinglocation.txt: " << line << RESET
					  << '\n';
			continue;
		}
		int match = findBestMatchLocation(uri, vServer);
		bool result = expected == match;
		displayResult(uri, result);
		if (!result) {
			std::cerr << "Expected: " << expected << " | found: " << match << '\n';
		}
	}
}

void testFinalUri() {
	Server server;
	server.parseConfig("./conf/valid/testmatching.conf");
	VirtualServer& vServer = server.getVirtualServers()[0];
	displayTitle("FINAL URI");
	std::ifstream config("./tests/testsfinaluri.txt");
	if (!config.good()) {
		std::cerr << "Cannot open file for testing final uri\n";
		return;
	}
	for (std::string line; std::getline(config, line);) {
		if (line[0] == '#' || line.empty()) {
			continue;
		}
		std::istringstream iss(line);
		std::string uri;
		std::string expectedFinalUri;
		if (!(iss >> uri >> expectedFinalUri)) {
			std::cerr << RED << "Invalid line in testmatchinglocation.txt: " << line << RESET
					  << '\n';
			continue;
		}
		int locnb = findBestMatchLocation(uri, vServer);
		std::string finalUri;
		if (locnb == -1) {
			finalUri = findFinalUri(uri, vServer.getRootDir(), NULL);
		} else {
			Location loc = vServer.getLocations()[locnb];
			finalUri = findFinalUri(uri, loc.getRootDir(), &loc);
		}
		bool result = finalUri == expectedFinalUri;
		displayResult(uri, result);
		if (!result) {
			std::cerr << "Expected: " << expectedFinalUri << " | found: " << finalUri << '\n';
		}
	}
}