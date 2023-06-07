#include "../includes/webserv.hpp"

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