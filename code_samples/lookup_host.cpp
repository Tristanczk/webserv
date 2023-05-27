#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

void lookup_host(const std::string& host) {
	struct addrinfo hints, *res, *result;
	int errcode;
	char addrstr[100];
	void* ptr;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	errcode = getaddrinfo(host.c_str(), NULL, &hints, &result);
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
	if (errcode != 0)
		return;

	res = result;

	for (struct addrinfo* res = result; res != NULL; res = res->ai_next) {
		inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, 100);

		switch (res->ai_family) {
		case AF_INET:
			ptr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
			break;
		case AF_INET6:
			ptr = &((struct sockaddr_in6*)res->ai_addr)->sin6_addr;
			break;
		default:
			std::cerr << "Unknown ai_family " << res->ai_family << std::endl;
		}
		inet_ntop(res->ai_family, ptr, addrstr, 100);
		printf("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4, addrstr,
			   res->ai_canonname);
	}

	freeaddrinfo(result);
}

int main(void) {
	do {
		std::cout << "Type domain name:" << std::endl;
		std::string str;
		std::getline(std::cin, str);
		if (str.size() == 0)
			return EXIT_SUCCESS;
		lookup_host(str);
		std::cout << std::endl;
	} while (true);
}