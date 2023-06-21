#include <climits>
#include <cstddef>
#include <iostream>
#include <map>

int main(void) {
	std::string n = "123458465456456456456456456465456456456";
	std::size_t size = std::strtol(n.c_str(), NULL, 10);
	if (size == LONG_MAX)
		std::cout << "too big" << std::endl;
	else
		std::cout << size << std::endl;
}
