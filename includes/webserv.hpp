#pragma once

#include <arpa/inet.h>
#include <climits>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <map>
#include <netinet/in.h>
#include <regex.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#define DEFAULT_PORT 8080
#define BACKLOG 128 // TODO SOMAXCONN
#define MAX_CLIENTS 1024
#define BUFFER_SIZE_SERVER_LIMIT 1048576 // 1MB
#define BODY_SIZE_LIMIT 1073741824		 // 1GB
#define BUFFER_SIZE_SERVER 16384
#define BODY_SIZE 1048576
#define DEFAULT_ERROR 0

#define CONFIG_FILE_ERROR "Error in configuration file: "
#define DEFAULT_CONF "conf/valid/default.conf"

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLUE "\033[34m"

#define LOCATION_MATCH_EXACT -2
#define LOCATION_MATCH_REGEX -1
#define LOCATION_MATCH_NONE 0

typedef enum e_vsmatch {
	VS_MATCH_NONE = 0,
	VS_MATCH_INADDR_ANY,
	VS_MATCH_IP,
	VS_MATCH_SERVER,
	VS_MATCH_BOTH,
} t_vsmatch;

class SystemError : public std::runtime_error {
public:
	explicit SystemError(const char* funcName) : std::runtime_error(funcName), funcName(funcName) {}
	virtual ~SystemError() throw() {}
	const char* funcName;
};

class RegexError : public std::exception {
public:
	virtual const char* what() const throw() { return "Error when using regex module"; }
};

class Location;
class Server;
class VirtualServer;

int comparePrefix(const std::string&, const std::string&);
bool endswith(const std::string&, const std::string&);
bool doesRegexMatch(const char* regexStr, const char* matchStr);
std::string getIpString(in_addr_t ip);
bool getIpValue(std::string, uint32_t&);

#include "Location.hpp"

#include "VirtualServer.hpp"

#include "Server.hpp"
