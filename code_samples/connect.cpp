#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int main() {
	// Create a socket
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == -1) {
		std::cerr << "Failed to create socket." << std::endl;
		return 1;
	}

	// Set up the server address and port
	sockaddr_in serverAddress;
	std::memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8080); // Assuming the server is listening on port 8080
	if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) <= 0) {
		std::cerr << "Invalid address/Address not supported." << std::endl;
		return 1;
	}

	// Connect to the server
	if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		std::cerr << "Connection failed." << std::endl;
		return 1;
	}

	std::cout << "Connected to the server!" << std::endl;

	// Close the socket
	close(clientSocket);

	return 0;
}
