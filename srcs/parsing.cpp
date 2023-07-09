#include "../includes/webserv.hpp"

bool parseRoot(std::istringstream& iss, std::string& root, const std::string& block) {
	std::string value;
	if (!(iss >> value))
		return configFileError("missing information after " + block + " root keyword");
	root = value;
	if (iss >> value)
		return configFileError("too many arguments after " + block + " root keyword");
	return true;
}

bool parseAutoIndex(std::istringstream& iss, bool& autoIndex) {
	std::string value;
	if (!(iss >> value))
		return configFileError("missing information after autoindex keyword");
	if (value == "on")
		autoIndex = true;
	else if (value == "off")
		autoIndex = false;
	else
		return configFileError("invalid value for autoindex keyword: " + value);
	if (iss >> value)
		return configFileError("too many arguments after autoindex keyword");
	return true;
}

bool parseErrorCode(std::string& code, std::vector<int>& codeList) {
	size_t idx = code.find_first_not_of("0123456789");
	if (idx != std::string::npos) {
		if (code[idx] == '-') {
			code[idx] = ' ';
			std::istringstream range(code);
			int start, end;
			std::string check;
			if (!(range >> start >> end))
				return configFileError("invalid format for code range in error_page");
			if (range >> check)
				return configFileError("too many arguments for code range in error_page");
			code[idx] = '-';
			if (!isValidErrorCode(start) || !isValidErrorCode(end))
				return configFileError("invalid error code in range: " + code);
			if (start > end)
				return configFileError("decreasing error code range: " + code);
			for (int i = start; i <= end; i++)
				codeList.push_back(i);
			return true;
		} else
			return configFileError("invalid error code: " + code);
	}
	int codeValue = std::strtol(code.c_str(), NULL, 10);
	if (!isValidErrorCode(codeValue))
		return configFileError("invalid error code: " + code);
	codeList.push_back(codeValue);
	return true;
}

bool parseErrorPages(std::istringstream& iss, std::map<int, std::string>& errorPages) {
	std::string code;
	std::string tmpStr;
	std::vector<int> codeList;
	if (!(iss >> code))
		return configFileError("missing information after error_page keyword");
	if (!parseErrorCode(code, codeList))
		return false;
	if (!(iss >> tmpStr))
		return configFileError("missing path after error_page code");
	code = tmpStr;
	while (iss >> tmpStr) {
		if (!parseErrorCode(code, codeList))
			return false;
		code = tmpStr;
	}
	for (std::vector<int>::iterator it = codeList.begin(); it != codeList.end(); it++) {
		int codeValue = *it;
		if (errorPages.find(codeValue) == errorPages.end()) {
			errorPages[codeValue] = code;
			// we replace the value only if the key does not exist, else it is the
			// first defined error page that is taken into account
		}
	}
	return true;
}

bool parseIndex(std::istringstream& iss, std::vector<std::string>& indexPages) {
	std::string value;
	if (!(iss >> value))
		return configFileError("missing information after index keyword");
	indexPages.push_back(value);
	while (iss >> value)
		indexPages.push_back(value);
	return true;
}

bool parseReturn(std::istringstream& iss, std::pair<long, std::string>& redirection) {
	std::string value;
	if (redirection.first != -1)
		return configFileError("multiple return instructions");
	if (!(iss >> value))
		return configFileError("missing information after return keyword");
	size_t idx = value.find_first_not_of("0123456789");
	if (idx != std::string::npos) {
		redirection.first = 302;
		redirection.second = value;
	} else {
		redirection.first = std::strtol(value.c_str(), NULL, 10);
		if (redirection.first < 300 || redirection.first > 399)
			return configFileError("invalid return code: " + toString(redirection.first));
		if (!(iss >> value))
			return configFileError("missing redirection url or text after return code");
		redirection.second = value;
	}
	if (iss >> value)
		return configFileError("too many arguments after return keyword");
	return true;
}