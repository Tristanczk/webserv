// #pragma once

// #include <arpa/inet.h>
// #include <csignal>
// #include <cstdio>
// #include <cstdlib>
// #include <cstring>
// #include <iostream>
// #include <netinet/in.h>
// #include <sys/epoll.h>
// #include <sys/socket.h>
// #include <sys/types.h>

// #define BACKLOG 128
// #define MAX_CLIENTS 1024
// #define BUFFER_SIZE 1024
// #define PORT 8888

// #define RESET "\033[0m"
// #define RED "\033[31m"
// #define GREEN "\033[32m"
// #define BLUE "\033[34m"

// class WSGIServer {
// public:
// 	WSGIServer(std::string serverAdress);
// 	~WSGIServer();

// private:
// 	int _listenSocket;
// 	int _reuseAddr;
// 	struct sockaddr_in _serverSocket;
// };