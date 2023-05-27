#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netdb.h>

int main() {
	struct protoent* protocol = getprotobyname("tcp");
	if (protocol != NULL) {
		std::cout << "Protocol Name: " << protocol->p_name << std::endl;
		for (char** alias = protocol->p_aliases; *alias != NULL; ++alias)
			std::cout << "Protocol Alias: " << protocol->p_aliases[0] << std::endl;
		std::cout << "Protocol Number: " << protocol->p_proto << std::endl;
	}
}
