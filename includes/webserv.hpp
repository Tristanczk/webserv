#pragma once

#include <arpa/inet.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "VirtualServer.hpp"

#define BACKLOG 128
#define MAX_CLIENTS 1024
#define BUFFER_SIZE 1024
#define PORT 8888

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLUE "\033[34m"

class SystemError : public std::runtime_error {
public:
	explicit SystemError(const char* funcName) : std::runtime_error(funcName), funcName(funcName) {}
	virtual ~SystemError() throw() {}
	const char* funcName;
};

bool parseConfig(std::string& file, std::vector<VirtualServer>& servers);
bool getIPvalue(std::string &IP, uint32_t &res);