#pragma once

#include <algorithm>
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
#include <iterator>
#include <map>
#include <netinet/in.h>
#include <queue>
#include <regex.h>
#include <set>
#include <sstream>
#include <stdexcept>
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
#define MAX_EVENTS 1024
#define MAX_URI_SIZE 2048
#define SIZE_LIMIT 33554432
#define BUFFER_SIZE 16384
#define DEFAULT_BODY_SIZE 1048576
#define RESPONSE_BUFFER_SIZE 1048576
#define MAX_HEADER_SIZE 1048576

#define TIMEOUT 10.0

#define DEBUG true

#define CGI_VERSION "CGI/1.1"
#define HTTP_VERSION "HTTP/1.1"
#define SERVER_VERSION "webserv/4.2"
#define DEFAULT_CONTENT_TYPE "application/octet-stream"

#define ERROR_ADDRESS "invalid IPv4 address format in listen instruction"
#define ERROR_LISTEN_FORMAT "invalid format for host:port in listen instruction"
#define ERROR_LOCATION_FORMAT                                                                      \
	"wrong syntax for location, syntax must be 'location [modifier] uri {'"
#define ERROR_PORT "invalid port number in listen instruction"

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"

#define SPACES " \f\n\r\t\v"

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

typedef enum RequestMethod { GET = 0, POST, DELETE, HEAD, NO_METHOD } RequestMethod;

typedef enum StatusCode {
	STATUS_NONE = 0,
	STATUS_CONTINUE = 100,
	STATUS_SWITCHING_PROTOCOLS = 101,
	STATUS_PROCESSING = 102,
	STATUS_EARLY_HINTS = 103,
	STATUS_OK = 200,
	STATUS_CREATED = 201,
	STATUS_ACCEPTED = 202,
	STATUS_NON_AUTHORITATIVE_INFORMATION = 203,
	STATUS_NO_CONTENT = 204,
	STATUS_RESET_CONTENT = 205,
	STATUS_PARTIAL_CONTENT = 206,
	STATUS_MULTI_STATUS = 207,
	STATUS_ALREADY_REPORTED = 208,
	STATUS_IM_USED = 226,
	STATUS_MULTIPLE_CHOICES = 300,
	STATUS_MOVED_PERMANENTLY = 301,
	STATUS_FOUND = 302,
	STATUS_SEE_OTHER = 303,
	STATUS_NOT_MODIFIED = 304,
	STATUS_USE_PROXY = 305,
	STATUS_TEMPORARY_REDIRECT = 307,
	STATUS_PERMANENT_REDIRECT = 308,
	STATUS_BAD_REQUEST = 400,
	STATUS_UNAUTHORIZED = 401,
	STATUS_PAYMENT_REQUIRED = 402,
	STATUS_FORBIDDEN = 403,
	STATUS_NOT_FOUND = 404,
	STATUS_METHOD_NOT_ALLOWED = 405,
	STATUS_NOT_ACCEPTABLE = 406,
	STATUS_PROXY_AUTHENTICATION_REQUIRED = 407,
	STATUS_REQUEST_TIMEOUT = 408,
	STATUS_CONFLICT = 409,
	STATUS_GONE = 410,
	STATUS_LENGTH_REQUIRED = 411,
	STATUS_PRECONDITION_FAILED = 412,
	STATUS_PAYLOAD_TOO_LARGE = 413,
	STATUS_URI_TOO_LONG = 414,
	STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
	STATUS_RANGE_NOT_SATISFIABLE = 416,
	STATUS_EXPECTATION_FAILED = 417,
	STATUS_IM_A_TEAPOT = 418,
	STATUS_MISDIRECTED_REQUEST = 421,
	STATUS_UNPROCESSABLE_ENTITY = 422,
	STATUS_LOCKED = 423,
	STATUS_FAILED_DEPENDENCY = 424,
	STATUS_TOO_EARLY = 425,
	STATUS_UPGRADE_REQUIRED = 426,
	STATUS_PRECONDITION_REQUIRED = 428,
	STATUS_TOO_MANY_REQUESTS = 429,
	STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	STATUS_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
	STATUS_INTERNAL_SERVER_ERROR = 500,
	STATUS_NOT_IMPLEMENTED = 501,
	STATUS_BAD_GATEWAY = 502,
	STATUS_SERVICE_UNAVAILABLE = 503,
	STATUS_GATEWAY_TIMEOUT = 504,
	STATUS_HTTP_VERSION_NOT_SUPPORTED = 505,
	STATUS_VARIANT_ALSO_NEGOTIATES = 506,
	STATUS_INSUFFICIENT_STORAGE = 507,
	STATUS_LOOP_DETECTED = 508,
	STATUS_NOT_EXTENDED = 510,
	STATUS_NETWORK_AUTHENTICATION_REQUIRED = 511,
} StatusCode;

typedef enum RequestParsingEnum {
	REQUEST_PARSING_FAILURE,
	REQUEST_PARSING_PROCESSING,
	REQUEST_PARSING_SUCCESS,
} RequestParsingEnum;

typedef struct RequestParsingSuccess {
	RequestMethod method;
	std::string uri;
	std::string query;
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
	DIRECTORY,
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
std::string findFinalUri(std::string& uri, std::string rootDir, Location* location);
std::string fullRead(int);
std::string getAbsolutePath(const std::string&);
std::string getBasename(const std::string&);
std::string getDate();
std::string getExtension(const std::string&);
std::string getIpString(in_addr_t);
bool getIpValue(std::string, uint32_t&);
void initAllowedMethods(bool[NO_METHOD]);
bool isDirectory(const std::string&);
bool isValidFile(const std::string&);
bool isValidErrorCode(int);
void perrored(const char*);
bool readContent(std::string&, std::string&);
std::string removeDuplicateSlashes(const std::string&);
bool startswith(const std::string&, const std::string&);
std::string strjoin(const std::vector<std::string>&, const std::string&);
std::string strlower(const std::string&);
std::string strtrim(const std::string&, const std::string&);
std::string strupper(const std::string&);
std::string toString(RequestMethod);
bool validateUri(const std::string&, const std::string& = "");
char** vectorToCharArray(const std::vector<std::string>&);

void mainDestructor();
void signalHandler(int);
void syscall(long, const char*);
void syscallEpoll(int, int, int, int, const char*);

bool parseAutoIndex(std::istringstream&, bool&);
bool parseDirectory(std::istringstream&, std::string&, const std::string&, const std::string&);
bool parseErrorPages(std::istringstream&, std::map<int, std::string>&);
bool parseIndex(std::istringstream&, std::vector<std::string>&);
bool parseReturn(std::istringstream&, std::pair<long, std::string>&);

void initGlobals();

#include "Location.hpp"

#include "VirtualServer.hpp"

#include "Request.hpp"

#include "Response.hpp"

#include "Client.hpp"

#include "Server.hpp"
