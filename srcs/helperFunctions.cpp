#include "../includes/webserv.hpp"

bool getIPvalue(std::string& IP, uint32_t& res) {
	int count = 0;
	size_t idx;
	if (IP == "localhost") {
		res = htonl(INADDR_LOOPBACK);
		return true;
	}
	if (IP == "*") {
		res = htonl(INADDR_ANY);
		return true;
	}
	while (idx = IP.find('.'), idx != std::string::npos) {
		IP.replace(idx, 1, 1, ' ');
		count++;
	}
	if (count != 3)
		return false;
	std::istringstream iss(IP);
	std::string check;
	int val[4];
	if (!(iss >> val[0] >> val[1] >> val[2] >> val[3]))
		return false;
	for (int i = 0; i < 4; i++) {
		if (val[i] < 0 || val[i] > 255)
			return false;
		res += val[i] << (8 * (3 - i));
	}
	if (iss >> check)
		return false;
	res = htonl(res);
	return true;
}