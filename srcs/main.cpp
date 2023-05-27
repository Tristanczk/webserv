#include "../includes/webserv.hpp"
#include <sys/socket.h>

static void syscall(int returnValue, const char* funcName) {
	if (returnValue < 0)
		throw SystemError(funcName);
}

static std::string fullRead(int fd) {
	std::string message;
	char buf[BUFFER_SIZE];

	while (true) {
		int buflen;
		syscall(buflen = read(fd, buf, BUFFER_SIZE - 1), "read"); // TODO recv
		buf[buflen] = '\0';
		message += buf;
		if (buflen < BUFFER_SIZE - 1)
			return message;
	}
}

const char* HTTP_RESPONSE = "HTTP/1.1 200 OK\n"
							"\n"
							"Hello, World?\n";

int main() {
	struct sockaddr_in serverSocket;
	std::memset(&serverSocket, 0, sizeof(serverSocket));
	serverSocket.sin_family = AF_INET;
	serverSocket.sin_port = htons(PORT);
	serverSocket.sin_addr.s_addr = htonl(INADDR_ANY);
	int serverFd;
	syscall(serverFd = socket(AF_INET, SOCK_STREAM, 0), "socket");
	int reuseAddr = 1;
	syscall(setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)),
			"setsockopt");
	syscall(bind(serverFd, (struct sockaddr*)&serverSocket, sizeof(serverSocket)), "bind");
	syscall(listen(serverFd, BACKLOG), "listen");
	int clientFd;

	std::cout << BLUE << "Listening on port " << PORT << ". 👂" << RESET << std::endl;

	while (true) {
		struct sockaddr_in clientSocket;
		socklen_t socklen = sizeof(clientSocket);
		std::memset(&clientSocket, 0, socklen);
		syscall(clientFd = accept(serverFd, (struct sockaddr*)&clientSocket, &socklen), "accept");
		std::string msg = fullRead(clientFd);
		while (msg != "quit\r\n") {
			std::cout << msg << std::endl;
			// std::cout << msg.size() << std::endl;
			if (msg == "send\r\n")
				send(clientFd, HTTP_RESPONSE, strlen(HTTP_RESPONSE), MSG_NOSIGNAL);
			msg = fullRead(clientFd);
		}
		close(clientFd);
	}
}