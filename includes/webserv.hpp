#pragma once

#include <arpa/inet.h>
#include <climits>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <netinet/in.h>
#include <queue>
#include <regex.h>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define DEFAULT_PORT 8080
#define MAX_PORT 65535
#define MAX_CLIENTS 1024
#define MAX_URI_SIZE 2048
#define SIZE_LIMIT 1073741824 // TODO test
#define BUFFER_SIZE 16384
#define DEFAULT_BODY_SIZE 1048576
#define MAX_HEADER_SIZE 1048576

#define SERVER_VERSION "webserv/4.2"

#define ERROR_ADDRESS "invalid IPv4 address format in listen instruction"
#define ERROR_LISTEN_FORMAT "invalid format for host:port in listen instruction"
#define ERROR_LOCATION "wrong syntax for location, syntax must be 'location [modifier] uri {'"
#define ERROR_PORT "invalid port number in listen instruction"

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLUE "\033[34m"

#define LOCATION_MATCH_EXACT -2
#define LOCATION_MATCH_REGEX -1
#define LOCATION_MATCH_NONE 0

class Client;
class Location;
class Request;
class Response;
class Server;
class VirtualServer;

template <typename T>
std::string toString(T x) {
	std::stringstream ss;
	ss << x;
	return ss.str();
}

typedef enum VirtualServerMatch {
	VS_MATCH_NONE = 0,
	VS_MATCH_INADDR_ANY,
	VS_MATCH_IP,
	VS_MATCH_SERVER,
	VS_MATCH_BOTH,
} VirtualServerMatch;

typedef enum RequestMethod { GET = 0, POST, DELETE, NO_METHOD } RequestMethod;

typedef enum StatusCode {
	NO_STATUS_CODE = 0,
	INFORMATIONAL_CONTINUE = 100,
	INFORMATIONAL_SWITCHING_PROTOCOLS = 101,
	INFORMATIONAL_PROCESSING = 102,
	INFORMATIONAL_EARLY_HINTS = 103,
	SUCCESS_OK = 200,
	SUCCESS_CREATED = 201,
	SUCCESS_ACCEPTED = 202,
	SUCCESS_NON_AUTHORITATIVE_INFORMATION = 203,
	SUCCESS_NO_CONTENT = 204,
	SUCCESS_RESET_CONTENT = 205,
	SUCCESS_PARTIAL_CONTENT = 206,
	SUCCESS_MULTI_STATUS = 207,
	SUCCESS_ALREADY_REPORTED = 208,
	SUCCESS_IM_USED = 226,
	REDIRECTION_MULTIPLE_CHOICES = 300,
	REDIRECTION_MOVED_PERMANENTLY = 301,
	REDIRECTION_FOUND = 302,
	REDIRECTION_SEE_OTHER = 303,
	REDIRECTION_NOT_MODIFIED = 304,
	REDIRECTION_USE_PROXY = 305,
	REDIRECTION_TEMPORARY_REDIRECT = 307,
	REDIRECTION_PERMANENT_REDIRECT = 308,
	CLIENT_BAD_REQUEST = 400,
	CLIENT_UNAUTHORIZED = 401,
	CLIENT_PAYMENT_REQUIRED = 402,
	CLIENT_FORBIDDEN = 403,
	CLIENT_NOT_FOUND = 404,
	CLIENT_METHOD_NOT_ALLOWED = 405,
	CLIENT_NOT_ACCEPTABLE = 406,
	CLIENT_PROXY_AUTHENTICATION_REQUIRED = 407,
	CLIENT_REQUEST_TIMEOUT = 408,
	CLIENT_CONFLICT = 409,
	CLIENT_GONE = 410,
	CLIENT_LENGTH_REQUIRED = 411,
	CLIENT_PRECONDITION_FAILED = 412,
	CLIENT_PAYLOAD_TOO_LARGE = 413,
	CLIENT_URI_TOO_LONG = 414,
	CLIENT_UNSUPPORTED_MEDIA_TYPE = 415,
	CLIENT_RANGE_NOT_SATISFIABLE = 416,
	CLIENT_EXPECTATION_FAILED = 417,
	CLIENT_IM_A_TEAPOT = 418,
	CLIENT_MISDIRECTED_REQUEST = 421,
	CLIENT_UNPROCESSABLE_ENTITY = 422,
	CLIENT_LOCKED = 423,
	CLIENT_FAILED_DEPENDENCY = 424,
	CLIENT_TOO_EARLY = 425,
	CLIENT_UPGRADE_REQUIRED = 426,
	CLIENT_PRECONDITION_REQUIRED = 428,
	CLIENT_TOO_MANY_REQUESTS = 429,
	CLIENT_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	CLIENT_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
	SERVER_INTERNAL_SERVER_ERROR = 500,
	SERVER_NOT_IMPLEMENTED = 501,
	SERVER_BAD_GATEWAY = 502,
	SERVER_SERVICE_UNAVAILABLE = 503,
	SERVER_GATEWAY_TIMEOUT = 504,
	SERVER_HTTP_VERSION_NOT_SUPPORTED = 505,
	SERVER_VARIANT_ALSO_NEGOTIATES = 506,
	SERVER_INSUFFICIENT_STORAGE = 507,
	SERVER_LOOP_DETECTED = 508,
	SERVER_NOT_EXTENDED = 510,
	SERVER_NETWORK_AUTHENTICATION_REQUIRED = 511,
} StatusCode;

typedef enum RequestParsingEnum {
	REQUEST_PARSING_FAILURE,
	REQUEST_PARSING_PROCESSING,
	REQUEST_PARSING_SUCCESS,
} RequestParsingEnum;

typedef struct RequestParsingSuccess {
	RequestMethod method;
	std::string uri;
	std::map<std::string, std::string> headers;
	std::vector<unsigned char> body;
} RequestParsingSuccess;

typedef struct RequestParsingResult {
	RequestParsingEnum result;
	StatusCode statusCode;
	RequestParsingSuccess success;
	VirtualServer* virtualServer;
	Location* location;
} RequestParsingResult;

typedef enum ResponseStatusEnum {
	RESPONSE_FAILURE,
	RESPONSE_PENDING,
	RESPONSE_SUCCESS,
} ResponseStatusEnum;

typedef enum LocationModifierEnum {
	NONE,
	REGEX,
	EXACT,
} LocationModifierEnum;

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
bool configFileError(const std::string&);
bool doesRegexMatch(const char*, const char*);
bool endswith(const std::string&, const std::string&);
const std::string* findCommonString(const std::vector<std::string>&,
									const std::vector<std::string>&);
std::string fullRead(int);
std::string getBasename(const std::string&);
std::string getExtension(const std::string& path);
std::string getDate();
std::string getIpString(in_addr_t);
bool getIpValue(std::string, uint32_t&);
bool getValidPath(std::string, char* const[], std::string&);
bool isDirectory(const std::string&);
bool isValidFile(const std::string& path);
bool isValidErrorCode(int);
bool readContent(std::string&, std::string&);
bool startswith(const std::string&, const std::string&);
std::string strlower(const std::string&);
std::string strtrim(const std::string&, const std::string&);
bool validateUrl(const std::string&, const std::string&);

void addEpollEvent(int, int, int);
void modifyEpollEvent(int, int, int);

void signalHandler(int);
void syscall(int, const char*);

bool parseAutoIndex(std::istringstream&, bool&);
bool parseErrorCode(std::string&, std::vector<int>&);
bool parseErrorPages(std::istringstream&, std::map<int, std::string>&);
bool parseIndex(std::istringstream&, std::vector<std::string>&);
bool parseReturn(std::istringstream&, std::pair<long, std::string>&);
bool parseString(std::istringstream&, std::string&, const std::string&);

#include "Location.hpp"

#include "VirtualServer.hpp"

#include "Request.hpp"

#include "Response.hpp"

#include "Client.hpp"

#include "Server.hpp"
