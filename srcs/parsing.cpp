#include "../includes/webserv.hpp"

bool parseRoot(std::istringstream& iss, std::string& rootDir) {
	std::string value;
	if (!(iss >> value)) {
		std::cerr << CONFIG_FILE_ERROR << "Missing information after root keyword" << std::endl;
		return false;
	}
	rootDir = value;
	if (iss >> value) {
		std::cerr << CONFIG_FILE_ERROR << "Too many arguments after root keyword" << std::endl;
		return false;
	}
	return true;
}

bool parseAutoIndex(std::istringstream& iss, bool& autoIndex) {
	std::string value;
	if (!(iss >> value)) {
		std::cerr << CONFIG_FILE_ERROR << "Missing information after autoindex keyword"
				  << std::endl;
		return false;
	}
	if (value == "on")
		autoIndex = true;
	else if (value == "off")
		autoIndex = false;
	else {
		std::cerr << CONFIG_FILE_ERROR << "Invalid value for autoindex keyword: " << value
				  << std::endl;
		return false;
	}
	if (iss >> value) {
		std::cerr << CONFIG_FILE_ERROR << "Too many arguments after autoindex keyword" << std::endl;
		return false;
	}
	return true;
}

bool parseErrorCode(std::string& code, std::vector<int>& codeList) {
	int codeValue;
	size_t idx = code.find_first_not_of("0123456789");
	if (idx != std::string::npos) {
		if (code[idx] == '-') {
			code[idx] = ' ';
			std::istringstream range(code);
			int start, end;
			std::string check;
			if (!(range >> start >> end)) {
				std::cerr << CONFIG_FILE_ERROR << "Invalid format for code range in error_page"
						  << std::endl;
				return false;
			}
			if (range >> check) {
				std::cerr << CONFIG_FILE_ERROR << "Too many arguments for code range in error_page"
						  << std::endl;
				return false;
			}
			if (start < 100 || start > 599 || end < 100 || end > 599) {
				std::cerr << CONFIG_FILE_ERROR << "Invalid error code in range: " << start << "-"
						  << end << std::endl;
				return false;
			}
			for (int i = start; i <= end; i++)
				codeList.push_back(i);
			return true;
		} else {
			std::cerr << CONFIG_FILE_ERROR << "Invalid character in error code: " << code
					  << std::endl;
			return false;
		}
	}
	codeValue = std::strtol(code.c_str(), NULL, 10);
	if (codeValue < 100 || codeValue > 599) {
		std::cerr << CONFIG_FILE_ERROR << "Invalid error code: " << codeValue << std::endl;
		return false;
	}
	codeList.push_back(codeValue);
	return true;
}

bool parseErrorPages(std::istringstream& iss, std::map<int, std::string>& errorPages) {
	std::string code;
	std::string tmpStr;
	std::vector<int> codeList;
	int codeValue;
	if (!(iss >> code)) {
		std::cerr << CONFIG_FILE_ERROR << "Missing information after error_page keyword"
				  << std::endl;
		return false;
	}
	if (!parseErrorCode(code, codeList))
		return false;
	if (!(iss >> tmpStr)) {
		std::cerr << CONFIG_FILE_ERROR << "Missing path after error_page code" << std::endl;
		return false;
	}
	code = tmpStr;
	while (iss >> tmpStr) {
		if (!parseErrorCode(code, codeList))
			return false;
		code = tmpStr;
	}
	for (std::vector<int>::iterator it = codeList.begin(); it != codeList.end(); it++) {
		codeValue = *it;
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
	if (!(iss >> value)) {
		std::cerr << CONFIG_FILE_ERROR << "Missing information after index keyword" << std::endl;
		return false;
	}
	indexPages.push_back(value);
	while (iss >> value)
		indexPages.push_back(value);
	return true;
}

bool parseReturn(std::istringstream& iss, std::pair<long, std::string>& redirection) {
	std::string value;
	if (redirection.first != -1) {
		std::cerr << CONFIG_FILE_ERROR << "Multiple return instructions" << std::endl;
		return false;
	}
	if (!(iss >> value)) {
		std::cerr << CONFIG_FILE_ERROR << "Missing information after return keyword" << std::endl;
		return false;
	}
	size_t idx = value.find_first_not_of("0123456789");
	if (idx != std::string::npos) {
		redirection.first = 302;
		redirection.second = value;
	} else {
		redirection.first = std::strtol(value.c_str(), NULL, 10);
		if (redirection.first == LONG_MAX) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid value for return code" << std::endl;
			return false;
		}
		if (!(redirection.first >= 0 && redirection.first <= 599)) {
			std::cerr << CONFIG_FILE_ERROR << "Invalid return code: " << redirection.first
					  << std::endl;
			return false;
		}
		if (!(iss >> value)) {
			std::cerr << CONFIG_FILE_ERROR << "Missing redirection url or text after return code"
					  << std::endl;
			return false;
		}
		redirection.second = value;
	}
	if (iss >> value) {
		std::cerr << CONFIG_FILE_ERROR << "Too many arguments after return keyword" << std::endl;
		return false;
	}
	return true;
}