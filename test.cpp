#include <iostream>
#include <arpa/inet.h>
#include <sstream>

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
	return true;
}

int main(void)
{
	std::string ip3 = "192.0.199999999999999999999999999999999999.1";
	struct sockaddr_in _address;
	if (inet_pton(AF_INET, ip3.c_str(), &_address.sin_addr) != 1)
	{
		std::cerr << "error" << std::endl;
		return EXIT_FAILURE;
	}
	uint32_t res = 0;
	if (!getIPvalue(ip3, res))
	{
		std::cerr << "error" << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << "address with inet_pton: " << _address.sin_addr.s_addr << std::endl;
	std::cout << "address with getIPvalue: " << res << std::endl;
	std::cout << "address with getIPvalue using htonl: " << htonl(res) << std::endl;
	return EXIT_SUCCESS;
}