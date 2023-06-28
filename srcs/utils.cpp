#include "../includes/webserv.hpp"

bool configFileError(const std::string& message) {
	std::cerr << CONFIG_FILE_ERROR << message << std::endl;
	return false;
}

int comparePrefix(const std::string& s1, const std::string& s2) {
	int i = 0;
	while (s1[i] && s2[i] && s1[i] == s2[i])
		++i;
	return i;
}

// TODO catch RegexError instead of generic exception where this function is called
bool doesRegexMatch(const char* regexStr, const char* matchStr) {
	regex_t regex;
	if (regcomp(&regex, regexStr, REG_EXTENDED) != 0)
		throw RegexError();
	int regint = regexec(&regex, matchStr, 0, NULL, 0);
	regfree(&regex);
	return regint == 0;
}

bool endswith(const std::string& str, const std::string& end) {
	return str.length() >= end.length() &&
		   !str.compare(str.length() - end.length(), end.length(), end);
}

const std::string* findCommonString(const std::vector<std::string>& vec1,
									const std::vector<std::string>& vec2) {
	std::set<std::string> set1(vec1.begin(), vec1.end());
	for (std::vector<std::string>::const_iterator it = vec2.begin(); it != vec2.end(); ++it)
		if (set1.find(*it) != set1.end())
			return &*it;
	return NULL;
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

bool readHTML(std::string& uri, std::string& content) {
	if (isDirectory(uri))
		return false;
	std::ifstream file(uri.c_str());
	if (!file.good())
		return false;
	std::stringstream buffer;
	buffer << file.rdbuf();
	content = buffer.str();
	return true;
}

std::string getIpString(in_addr_t ip) {
	ip = ntohl(ip);
	std::ostringstream oss;
	oss << (ip >> 24 & 0xFF) << '.' << (ip >> 16 & 0xFF) << '.' << (ip >> 8 & 0xFF) << '.'
		<< (ip & 0xFF);
	return oss.str();
}

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

bool isDirectory(const std::string& path) {
	struct stat buf;
	return stat(path.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode);
}

bool isValidErrorCode(int errorCode) { return 100 <= errorCode && errorCode <= 599; }

bool getValidPath(std::string path, char* const envp[], std::string& finalPath) {
	if (path.find('/') != std::string::npos) {
		finalPath = path;
		return true;
	}
	int i = 0;
	std::string cmp = "PATH=";
	std::string pathEnv;
	while (envp[i]) {
		std::string env(envp[i]);
		if (env.substr(0, cmp.length()) == cmp) {
			pathEnv = env.substr(cmp.length());
			break;
		}
	}
	while (!pathEnv.empty()) {
		std::string pathToCheck = pathEnv.substr(0, pathEnv.find(':'));
		pathEnv = pathEnv.substr(pathToCheck.length() + 1);
		std::string fullPath = pathToCheck + '/' + path;
		if (access(fullPath.c_str(), X_OK) == 0) {
			finalPath = fullPath;
			return true;
		}
	}
	return false;
}

std::string getBasename(const std::string& path) {
	return path.substr(path.find_last_of("/\\") + 1);
}

std::string getDate() {
	std::time_t t = std::time(0);
	std::tm* now = std::localtime(&t);
	char buffer[256];
	std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", now);
	return std::string(buffer);
}
