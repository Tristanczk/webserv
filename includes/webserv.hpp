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
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define DEFAULT_PORT 8080
#define MAX_PORT 65535
#define MAX_CLIENTS 1024
#define BUFFER_SIZE_SERVER_LIMIT 1048576
#define SIZE_LIMIT 1073741824
#define BUFFER_SIZE_SERVER 16384
#define MIN_BUFFER_SIZE 1024
#define DEFAULT_SIZE 1048576
#define DEFAULT_ERROR 0

#define CONFIG_FILE_ERROR "Error in configuration file: "
#define ERROR_LOCATION "Wrong syntax for location, syntax must be 'location [modifier] uri {'"
#define DEFAULT_CONF "conf/valid/default.conf"

#define LOCATION_MATCH_EXACT -2
#define LOCATION_MATCH_REGEX -1
#define LOCATION_MATCH_NONE 0

template <typename T>
std::string toString(T x) {
	std::stringstream ss;
	ss << x;
	return ss.str();
}

typedef enum e_vsmatch {
	VS_MATCH_NONE = 0,
	VS_MATCH_INADDR_ANY,
	VS_MATCH_IP,
	VS_MATCH_SERVER,
	VS_MATCH_BOTH,
} t_vsmatch;

typedef enum RequestMethod { GET = 0, POST, DELETE, NO_METHOD } RequestMethod;

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

int comparePrefix(const std::string&, const std::string&);
bool configFileError(std::string);
bool endswith(const std::string&, const std::string&);
bool doesRegexMatch(const char*, const char*);
std::string fullRead(int, size_t);
std::string getIpString(in_addr_t);
bool getIpValue(std::string, uint32_t&);

bool parseAutoIndex(std::istringstream&, bool&);
bool parseErrorCode(std::string&, std::vector<int>&);
bool parseErrorPages(std::istringstream&, std::map<int, std::string>&);
bool parseIndex(std::istringstream&, std::vector<std::string>&);
bool parseReturn(std::istringstream&, std::pair<long, std::string>&);
bool parseRoot(std::istringstream&, std::string&);

class Location;
class VirtualServer;
class Client;
class Server;

#include "Location.hpp"

#include "VirtualServer.hpp"

#include "Client.hpp"

#include "Server.hpp"
