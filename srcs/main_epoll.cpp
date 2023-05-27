// #include "../includes/webserv.hpp"

// int epollFd, serverFd;
// struct epoll_event evlist[MAX_CLIENTS];

// static void clean() {
// 	for (int i = 0; i < MAX_CLIENTS; ++i)
// 		close(evlist[i].data.fd);
// 	close(serverFd);
// 	close(epollFd);
// }

// static void goodBye() {
// 	std::cout << "\rGood bye. 💞\n";
// 	clean();
// 	exit(EXIT_SUCCESS);
// }

// static void handleSigint(int signum) {
// 	(void)signum;
// 	goodBye();
// }

// static void syscall(int returnValue, const char* funcName) {
// 	if (returnValue == -1) {
// 		std::perror(funcName);
// 		clean();
// 		exit(EXIT_FAILURE);
// 	}
// }

// static std::string fullRead(int fd) {
// 	std::string message;
// 	char buf[BUFFER_SIZE];

// 	while (true) {
// 		int buflen;
// 		syscall(buflen = read(fd, buf, BUFFER_SIZE - 1), "read"); // TODO recv
// 		buf[buflen] = '\0';
// 		message += buf;
// 		if (buflen < BUFFER_SIZE - 1)
// 			return message;
// 	}
// }

// static void addEvent(int epollFd, int eventFd) {
// 	struct epoll_event evStdin;
// 	evStdin.events = EPOLLIN; // TODO other flags
// 	evStdin.data.fd = eventFd;
// 	syscall(epoll_ctl(epollFd, EPOLL_CTL_ADD, eventFd, &evStdin), "epoll_ctl");
// }

// static void initEpoll(char* port) {
// 	struct sockaddr_in serverSocket;
// 	std::memset(&serverSocket, 0, sizeof(serverSocket));
// 	serverSocket.sin_family = AF_INET;
// 	serverSocket.sin_port = htons(std::atoi(port));
// 	serverSocket.sin_addr.s_addr = htonl(INADDR_ANY);

// 	syscall(serverFd = socket(AF_INET, SOCK_STREAM, 0), "socket");
// 	int reuseAddr = 1;
// 	syscall(setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)),
// 			"setsockopt");
// 	syscall(bind(serverFd, (struct sockaddr*)&serverSocket, sizeof(serverSocket)), "bind");
// 	syscall(listen(serverFd, BACKLOG), "listen");
// 	syscall(epollFd = epoll_create1(0), "epoll_create1");
// 	addEvent(epollFd, STDIN_FILENO);
// 	addEvent(epollFd, serverFd);
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
// 			} else if (evlist[i].data.fd == serverFd) {
// 				struct sockaddr_in clientSocket;
// 				socklen_t socklen = sizeof(clientSocket);
// 				std::memset(&clientSocket, 0, socklen);
// 				syscall(clientFd = accept(serverFd, (struct sockaddr*)&clientSocket, &socklen),
// 						"accept");
// 				std::cout << "\x1b[0;32m[*] accept\x1b[0m\n";
// 				addEvent(epollFd, clientFd);
// 			} else {
// 				clientFd = evlist[i].data.fd;
// 				std::string msg = fullRead(clientFd);
// 				if (msg == "") {
// 					std::cout << "\x1b[0;31m[*] close\x1b[0m\n";
// 					syscall(epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, &evlist[i]), "epoll_ctl");
// 					close(clientFd);
// 				} else {
// 					std::cout << msg << std::endl;
// 					syscall(epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, &evlist[i]), "epoll_ctl");
// 					close(clientFd);
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
// 	std::signal(SIGINT, handleSigint);
// 	initEpoll(argv[1]);
// 	std::cout << "Listening on port " << argv[1] << ". 👂\n";
// 	loop();
// }