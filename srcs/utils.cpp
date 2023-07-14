#include "../includes/webserv.hpp"

int comparePrefix(const std::string& locationUri, const std::string& requestPath) {
	if (requestPath + "/" == locationUri) {
		return locationUri.size();
	}
	return startswith(requestPath, locationUri) ? locationUri.size() : 0;
}

bool configFileError(const std::string& message) {
	std::cerr << "Configuration error: " << message << std::endl;
	return false;
}

bool doesRegexMatch(const char* regexStr, const char* matchStr) {
	regex_t regex;
	if (regcomp(&regex, regexStr, REG_EXTENDED) != 0) {
		throw RegexError();
	}
	int regint = regexec(&regex, matchStr, 0, NULL, 0);
	return regint == 0;
}

bool endswith(const std::string& str, const std::string& end) {
	return str.size() >= end.size() && !str.compare(str.size() - end.size(), end.size(), end);
}

const std::string* findCommonString(const std::vector<std::string>& vec1,
									const std::vector<std::string>& vec2) {
	std::set<std::string> set1(vec1.begin(), vec1.end());
	for (std::vector<std::string>::const_iterator it = vec2.begin(); it != vec2.end(); ++it) {
		if (set1.find(*it) != set1.end()) {
			return &*it;
		}
	}
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
		if (buflen < BUFFER_SIZE - 1) {
			return message;
		}
	}
}

std::string getAbsolutePath(const std::string& path) {
	if (!startswith(path, "./")) {
		return "/";
	}
	char buffer[PATH_MAX];
	return getcwd(buffer, sizeof(buffer)) ? std::string(buffer) + "/" + path.substr(2) : "/";
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
	if (dots != 3) {
		return false;
	}
	std::istringstream iss(ip);
	std::string check;
	int val[4];
	if (!(iss >> val[0] >> val[1] >> val[2] >> val[3])) {
		return false;
	}
	res = 0;
	for (int i = 0; i < 4; ++i) {
		if (val[i] < 0 || val[i] > 255) {
			return false;
		}
		res = res << 8 | val[i];
	}
	if (iss >> check) {
		return false;
	}
	res = htonl(res);
	return true;
}

void initAllowedMethods(bool allowedMethods[NO_METHOD]) {
	std::fill_n(allowedMethods, NO_METHOD, false);
	allowedMethods[GET] = true;
	allowedMethods[HEAD] = true;
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

void perrored(const char* funcName) {
	std::cerr << RED << funcName << ": " << strerror(errno) << RESET << '\n';
}

bool readContent(std::string& uri, std::string& content) {
	if (isDirectory(uri)) {
		return false;
	}
	std::ifstream file(uri.c_str());
	if (!file.good()) {
		return false;
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	content = buffer.str();
	return true;
}

bool startswith(const std::string& str, const std::string& start) {
	return str.size() >= start.size() && !str.compare(0, start.size(), start);
}

std::string strjoin(const std::vector<std::string>& vec, const std::string& sep) {
	std::string result;
	for (std::vector<std::string>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
		if (it != vec.begin()) {
			result += sep;
		}
		result += *it;
	}
	return result;
}

std::string strlower(const std::string& s) {
	std::string l;
	for (size_t i = 0; i < s.size(); ++i) {
		l += std::tolower(s[i]);
	}
	return l;
}

std::string strtrim(const std::string& s, const std::string& remove) {
	std::string result = s;
	std::string::size_type pos = result.find_first_not_of(remove);
	if (pos == std::string::npos) {
		return result;
	}
	result.erase(0, pos);
	pos = result.find_last_not_of(remove);
	result.erase(pos + 1);
	return result;
}

std::string strupper(const std::string& s) {
	std::string l;
	for (size_t i = 0; i < s.size(); ++i) {
		l += std::toupper(s[i]);
	}
	return l;
}

std::string toString(RequestMethod method) {
	return method == GET ? "GET" : method == POST ? "POST" : "DELETE";
}

bool validateUri(const std::string& uri, const std::string& keyword) {
	return !uri.empty() && uri[0] == '/' && uri.find("..") == std::string::npos ? true
		   : keyword.empty()													? false
							 : configFileError("invalid path for " + keyword + ": " + uri);
}

char** vectorToCharArray(const std::vector<std::string>& envec) {
	char** array = new char*[envec.size() + 1];
	for (size_t i = 0; i < envec.size(); ++i) {
		array[i] = new char[envec[i].size() + 1];
		std::copy(envec[i].begin(), envec[i].end(), array[i]);
		array[i][envec[i].size()] = '\0';
	}
	array[envec.size()] = NULL;
	return array;
}

std::string findFinalUri(std::string& uri, std::string rootDir, Location* location) {
	if (rootDir[rootDir.size() - 1] == '/') {
		rootDir = rootDir.substr(0, rootDir.size() - 1);
	}
	if (!location) {
		return "." + rootDir + uri;
	}
	LocationModifierEnum modifier = location->getModifier();
	std::string locationUri = location->getUri();
	if (modifier == DIRECTORY) {
		return "." + rootDir + uri.substr(locationUri.size() - 1);
	} else if (modifier == REGEX) {
		return "." + rootDir + uri;
	} else {
		return "." + rootDir + "/" + getBasename(uri);
	}
}
