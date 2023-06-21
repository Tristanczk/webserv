#include "../includes/webserv.hpp"
#include <cstddef>

bool getIpValue(std::string ip, uint32_t& res) {
	if (ip == "localhost") {
		res = htonl(INADDR_LOOPBACK);
		return true;
	}
	if (ip == "*") {
		res = htonl(INADDR_ANY);
		return true;
	}
	size_t idx;
	int dots = 0;
	while ((idx = ip.find('.')) != std::string::npos) {
		ip.replace(idx, 1, 1, ' ');
		++dots;
	}
	if (dots != 3)
		return false;
	std::istringstream iss(ip);
	std::string check;
	int val[4];
	if (!(iss >> val[0] >> val[1] >> val[2] >> val[3]))
		return false;
	res = 0;
	for (int i = 0; i < 4; ++i) {
		if (val[i] < 0 || val[i] > 255)
			return false;
		res = res << 8 | val[i];
	}
	if (iss >> check)
		return false;
	res = htonl(res);
	return true;
}

std::string getIpString(in_addr_t ip) {
	ip = ntohl(ip);
	std::ostringstream oss;
	oss << (ip >> 24 & 0xFF) << '.' << (ip >> 16 & 0xFF) << '.' << (ip >> 8 & 0xFF) << '.'
		<< (ip & 0xFF);
	return oss.str();
}

int comparePrefix(const std::string& s1, const std::string& s2) {
	int i = 0;
	while (s1[i] && s2[i] && s1[i] == s2[i])
		++i;
	return i;
}

bool endswith(const std::string& str, const std::string& end) {
	return str.length() >= end.length() &&
		   !str.compare(str.length() - end.length(), end.length(), end);
}

bool doesRegexMatch(const char* regexStr, const char* matchStr) {
	regex_t regex;
	if (regcomp(&regex, regexStr, REG_EXTENDED) != 0)
		throw RegexError();
	int regint = regexec(&regex, matchStr, 0, NULL, 0);
	regfree(&regex);
	return regint == 0;
}

static void syscall(int returnValue, const char* funcName) {
	if (returnValue == -1) {
		std::perror(funcName);
		std::exit(EXIT_FAILURE);
	}
}

std::string fullRead(int fd, size_t bufferSize) {
	std::string message;
	char buf[bufferSize];
	size_t buflen;

	while (true) {
		syscall(buflen = read(fd, buf, bufferSize - 1), "read");
		buf[buflen] = '\0';
		message += buf;
		if (buflen < bufferSize - 1)
			return message;
	}
}