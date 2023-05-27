// #include "../includes/WSGIServer.hpp"

// WSGIServer::WSGIServer(std::string const serverAdress) {
// 	std::memset(&_serverSocket, 0, sizeof(_serverSocket));
// 	_serverSocket.sin_family = AF_INET;
// 	_serverSocket.sin_port = htons(PORT);
// 	_serverSocket.sin_addr.s_addr = htonl(INADDR_ANY);
// 	syscall(_listenSocket = socket(AF_INET, SOCK_STREAM, 0), "socket");
// 	syscall(setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, &_reuseAddr, sizeof(_reuseAddr)),
// 			"setsockopt");
// 	syscall(bind(_listenSocket, (struct sockaddr*)&_serverSocket, sizeof(_serverSocket)), "bind");
// 	syscall(listen(_listenSocket, BACKLOG), "listen");
// }