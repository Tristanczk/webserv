#include "../includes/webserv.hpp"

bool getIPvalue(std::string &IP, uint32_t &res){
	int count = 0;
	size_t idx;
	while (idx = IP.find('.'), idx != std::string::npos)
	{
		IP.replace(idx, 1, 1, ' ');
		std::cerr << "here" << std::endl;
		count++;
	}
	if (count != 3)
	{
		std::cerr << "Invalid IPv4 address format" << std::endl;
		return false;
	}
	std::istringstream iss(IP);
	int val[4];
	if (!(iss >> val[0] >> val[1] >> val[2] >> val[3]))
	{
		std::cerr << "Invalid IPv4 address format" << std::endl;
		return false;
	}
	for (int i = 0; i < 4; i++)
	{
		if (val[i] < 0 || val[i] > 255)
		{
			std::cerr << "Invalid IPv4 address format" << std::endl;
			return false;
		}
		res += val[i] << (8 * (3 - i));
	}
	res = htonl(res);
	return true;
}