#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#define MAX_HOST 1024

int main() {
	struct addrinfo* result;
	int error = getaddrinfo("chess.com", NULL, NULL, &result);
	if (error) {
		if (error == EAI_SYSTEM)
			perror("getaddrinfo");
		else
			fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
		return EXIT_FAILURE;
	}

	for (struct addrinfo* cur = result; cur != NULL; cur = cur->ai_next) {
		char hostname[MAX_HOST];
		if (getnameinfo(cur->ai_addr, cur->ai_addrlen, hostname, MAX_HOST, NULL, 0, 0))
			fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
		else
			printf("hostname: %s\n", hostname);
	}

	freeaddrinfo(result);
	return EXIT_SUCCESS;
}