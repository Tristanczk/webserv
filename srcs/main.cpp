#include "../includes/webserv.hpp"

// int epollFd, socketFd;
// struct epoll_event evlist[MAX_CLIENTS];

// static void clean() {
// 	for (int i = 0; i < MAX_CLIENTS; ++i)
// 		close(evlist[i].data.fd);
// 	close(socketFd);
// 	close(epollFd);
// }

// static void goodBye() {
// 	std::cout << "\rGood bye. 💞\n";
// 	clean();
// 	std::exit(EXIT_SUCCESS);
// }

// static void syscall(int returnValue, const char* funcName) {
// 	if (returnValue == -1) {
// 		std::perror(funcName);
// 		clean();
// 		std::exit(EXIT_FAILURE);
// 	}
// }

// static std::string fullRead(int fd) {
// 	std::string message;
// 	char buf[BUFFER_SIZE_IRC];

// 	while (true) {
// 		int buflen;
// 		syscall(buflen = read(fd, buf, BUFFER_SIZE_IRC - 1), "read");
// 		buf[buflen] = '\0';
// 		message += buf;
// 		if (buflen < BUFFER_SIZE_IRC - 1)
// 			return message;
// 	}
// }

// static void addEvent(int epollFd, int eventFd, int flags) {
// 	struct epoll_event ee;
// 	ee.events = flags;
// 	ee.data.fd = eventFd;
// 	syscall(epoll_ctl(epollFd, EPOLL_CTL_ADD, eventFd, &ee), "epoll_ctl");
// }

// static void initEpoll(char* port) {
// 	struct sockaddr_in serverSocket;
// 	std::memset(&serverSocket, 0, sizeof(serverSocket));
// 	serverSocket.sin_family = AF_INET;
// 	serverSocket.sin_port = htons(std::atoi(port));
// 	serverSocket.sin_addr.s_addr = htonl(INADDR_ANY);

// 	syscall(socketFd = socket(AF_INET, SOCK_STREAM, 0), "socket");
// 	syscall(bind(socketFd, (struct sockaddr*)&serverSocket, sizeof(serverSocket)), "bind");
// 	syscall(listen(socketFd, BACKLOG), "listen"); // TODO SOMAXCONN
// 	syscall(epollFd = epoll_create1(0), "epoll_create1");
// 	addEvent(epollFd, STDIN_FILENO, EPOLLIN);
// 	addEvent(epollFd, socketFd, EPOLLIN);
// }

// static void loop() {
// 	int clientFd, numFds;
// 	while (true) {
// 		syscall(numFds = epoll_wait(epollFd, evlist, MAX_CLIENTS, -1), "epoll_wait");
// 		for (int i = 0; i < numFds; ++i) {
// 			if (evlist[i].data.fd == STDIN_FILENO) {
// 				std::string input = fullRead(STDIN_FILENO);
// 				if (input == "quit\n")
// 					goodBye();
// 			} else if (evlist[i].data.fd == socketFd) {
// 				struct sockaddr_in clientSocket;
// 				bzero(&clientSocket, sizeof(clientSocket));
// 				socklen_t socklen = sizeof(clientSocket);
// 				syscall(clientFd = accept(socketFd, (struct sockaddr*)&clientSocket, &socklen),
// 						"accept");
// 				std::cout << "\x1b[0;32m[*] accept\x1b[0m\n";
// 				addEvent(epollFd, clientFd, EPOLLIN);
// 			} else {
// 				clientFd = evlist[i].data.fd;
// 				std::string msg = fullRead(clientFd);
// 				if (msg == "") {
// 					std::cout << "\x1b[0;31m[*] close\x1b[0m\n";
// 					syscall(epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, &evlist[i]), "epoll_ctl");
// 					close(clientFd);
// 				} else {
// 					std::cout << msg << std::endl;
// 				}
// 			}
// 		}
// 	}
// }

// int main(int argc, char* argv[]) {
// 	if (argc != 2) {
// 		std::cerr << "Usage: " << argv[0] << " port\n";
// 		return EXIT_FAILURE;
// 	}
// 	initEpoll(argv[1]);
// 	std::cout << "Listening on port " << argv[1] << ". 👂\n";
// 	loop();
// }

// main for testing parsing of config file
int main(int argc, char* argv[]) {
	if (argc > 2 || (argc == 2 && !endswith(argv[1], ".conf"))) {
		std::cerr << "Usage: " << argv[0] << " [filename.conf]" << std::endl;
		return EXIT_FAILURE;
	}
	Server server;
	if (!server.parseConfig(argc == 2 ? argv[1] : DEFAULT_CONF))
		return EXIT_FAILURE;
	// server.printVirtualServerList();
	VirtualServer* vs =
		server.findMatchingVirtualServer(htons(8080), htonl(INADDR_ANY), "website.com");
	if (vs == NULL) {
		std::cout << "No matching server found" << std::endl;
		return EXIT_FAILURE;
	} else {
		vs->printServerInformation();
	}
	Location* loc = vs->findMatchingLocation("/test.php");
	if (loc == NULL) {
		std::cout << "No matching location found" << std::endl;
		return EXIT_FAILURE;
	} else {
		loc->printLocationInformation();
	}
	return EXIT_SUCCESS;
}