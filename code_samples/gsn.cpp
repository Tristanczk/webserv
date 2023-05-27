#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static void printHostAndPort(int sockfd) {
	// Retrieve the local address and port using getsockname
	struct sockaddr_in localAddr;
	socklen_t addrLen = sizeof(localAddr);
	if (getsockname(sockfd, (struct sockaddr*)&localAddr, &addrLen) == -1)
		return std::perror("getsockname");

	// Convert the binary IP address to a human-readable format
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(localAddr.sin_addr), ip, INET_ADDRSTRLEN);

	// Print the local address and port
	std::cout << "Local address: " << ip << std::endl;
	std::cout << "Local port: " << ntohs(localAddr.sin_port) << std::endl;
}

static void bind8888(int sockfd) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(8888);
	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		std::cerr << "Failed to bind socket." << std::endl;
}

int main() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	printHostAndPort(sockfd);
	bind8888(sockfd);
	printHostAndPort(sockfd);
	close(sockfd);
}
