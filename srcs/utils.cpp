#include "../includes/webserv.hpp"

int comparePrefix(const std::string& locationUri, const std::string& requestPath) {
	return startswith(requestPath, locationUri) ? locationUri.size() : 0;
}

bool configFileError(const std::string& message) {
	std::cerr << "Configuration error: " << message << std::endl;
	return false;
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
	return str.size() >= end.size() && !str.compare(str.size() - end.size(), end.size(), end);
}

const std::string* findCommonString(const std::vector<std::string>& vec1,
									const std::vector<std::string>& vec2) {
	std::set<std::string> set1(vec1.begin(), vec1.end());
	for (std::vector<std::string>::const_iterator it = vec2.begin(); it != vec2.end(); ++it)
		if (set1.find(*it) != set1.end())
			return &*it;
	return NULL;
}

std::string fullRead(int fd) {
	std::string message;
	char buf[BUFFER_SIZE];
	size_t buflen;

	while (true) {
		syscall(buflen = read(fd, buf, BUFFER_SIZE - 1), "read");
		buf[buflen] = '\0';
		message += buf;
		if (buflen < BUFFER_SIZE - 1)
			return message;
	}
}

std::string getBasename(const std::string& path) { return path.substr(path.find_last_of("/") + 1); }

std::string getExtension(const std::string& path) {
	std::string basename = getBasename(path);
	size_t pos = basename.find_last_of(".");
	return pos == std::string::npos ? "" : basename.substr(pos + 1);
}

std::string getDate() {
	std::time_t t = std::time(0);
	std::tm* now = std::localtime(&t);
	char buffer[256];
	std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", now);
	return std::string(buffer);
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
		if (env.substr(0, cmp.size()) == cmp) {
			pathEnv = env.substr(cmp.size());
			break;
		}
	}
	while (!pathEnv.empty()) {
		std::string pathToCheck = pathEnv.substr(0, pathEnv.find(':'));
		pathEnv = pathEnv.substr(pathToCheck.size() + 1);
		std::string fullPath = pathToCheck + '/' + path;
		if (access(fullPath.c_str(), X_OK) == 0) {
			finalPath = fullPath;
			return true;
		}
	}
	return false;
}

void initAllowedMethods(bool allowedMethods[NO_METHOD]) {
	std::fill_n(allowedMethods, NO_METHOD, false);
	allowedMethods[GET] = true;
	allowedMethods[POST] = true;
}

bool isDirectory(const std::string& path) {
	struct stat buf;
	return stat(path.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode);
}

bool isValidFile(const std::string& path) {
	struct stat buf;
	return stat(path.c_str(), &buf) == 0 && S_ISREG(buf.st_mode);
}

bool isValidErrorCode(int errorCode) { return 100 <= errorCode && errorCode <= 599; }

bool readContent(std::string& uri, std::string& content) {
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

bool startswith(const std::string& str, const std::string& start) {
	return str.size() >= start.size() && !str.compare(0, start.size(), start);
}

std::string strlower(const std::string& s) {
	std::string l;
	for (size_t i = 0; i < s.size(); ++i)
		l += tolower(s[i]);
	return l;
}

std::string strtrim(const std::string& s, const std::string& remove) {
	std::string result = s;
	std::string::size_type pos = result.find_first_not_of(remove);
	if (pos == std::string::npos)
		return result;
	result.erase(0, pos);
	pos = result.find_last_not_of(remove);
	result.erase(pos + 1);
	return result;
}

bool validateUrl(const std::string& url, const std::string& keyword) {
	return url.empty() || url[0] != '/' || url.find("..") != std::string::npos
			   ? configFileError("invalid path for " + keyword + ": " + url)
			   : true;
}
