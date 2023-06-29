#pragma once

#include "webserv.hpp"

typedef enum ResponseStatusEnum {
	RESPONSE_FAILURE,
	RESPONSE_PENDING,
	RESPONSE_SUCCESS,
} ResponseStatusEnum;

class Response {
public:
	// constructor in case of non matching location block, there won't be a locationUri, a redirect,
	// allowedMethods or link to cgiExec as they are exclusively defined in the location block
	Response(std::string rootDir, bool autoIndex, std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages)
		: _rootDir(rootDir), _autoIndex(autoIndex), _errorPages(errorPages),
		  _indexPages(indexPages), _locationUri(""), _return(-1, ""), _cgiExec("") {
		for (int i = 0; i < NO_METHOD; ++i)
			_allowedMethods[i] = true;
		initKeywordMap();
		initStatusMessageMap();
		// TODO remove void
		(void)_allowedMethods;
		(void)_autoIndex;
	}

	Response(std::string rootDir, bool autoIndex, std::map<int, std::string> const& errorPages,
			 std::vector<std::string> const& indexPages, std::string locationUri,
			 std::pair<long, std::string> redirect, const bool allowedMethods[NO_METHOD],
			 std::string cgiExec)
		: _rootDir(rootDir), _autoIndex(autoIndex), _errorPages(errorPages),
		  _indexPages(indexPages), _locationUri(locationUri), _return(redirect), _cgiExec(cgiExec) {
		for (int i = 0; i < NO_METHOD; ++i)
			_allowedMethods[i] = allowedMethods[i];
		initKeywordMap();
		initStatusMessageMap();
		// TODO remove void
		(void)_allowedMethods;
		(void)_autoIndex;
	}

	Response(const Response& copy) {
		*this = copy;
		initKeywordMap();
		initStatusMessageMap();
	}
	~Response(){};

	Response& operator=(const Response& assign) {
		if (this == &assign)
			return *this;
		_statusLine = assign._statusLine;
		_headers = assign._headers;
		_tmpBody = assign._tmpBody;
		_body = assign._body;
		_bodyType = assign._bodyType;
		_statusCode = assign._statusCode;
		_rootDir = assign._rootDir;
		_autoIndex = assign._autoIndex;
		_errorPages = assign._errorPages;
		_indexPages = assign._indexPages;
		_locationUri = assign._locationUri;
		_return = assign._return;
		_cgiExec = assign._cgiExec;
		for (int i = 0; i < NO_METHOD; ++i)
			_allowedMethods[i] = assign._allowedMethods[i];
		return *this;
	}

	void buildResponse(RequestParsingResult& request) {
		if (request.result == REQUEST_PARSING_FAILURE) {
			_statusCode = request.statusCode;
			buildErrorPage();
			buildStatusLine();
			buildHeader();
		} else {
			_statusCode = SUCCESS_OK;
			if (!buildPage(request))
				buildErrorPage();
			buildStatusLine();
			buildHeader();
			std::cout << "Response built" << std::endl;
		}
	}

	bool pushResponseToClient(int fd) {
		std::string line;
		if (!pushLineToClient(fd, _statusLine))
			return false;
		for (std::map<std::string, std::string>::iterator it = _headers.begin();
			 it != _headers.end(); it++) {
			line = it->first + ": " + it->second + "\r\n";
			if (!pushLineToClient(fd, line))
				return false;
		}
		line = "\r\n";
		if (!pushLineToClient(fd, line))
			return false;
		for (std::vector<char>::iterator it = _tmpBody.begin(); it != _tmpBody.end(); it++) {
			if (send(fd, &(*it), 1, 0) == -1) {
				std::cerr << "Error: send failed" << std::endl;
				return false;
			}
		}
		return true;
	}

private:
	typedef bool (Response::*KeywordHandler)(RequestParsingResult&);
	std::map<RequestMethod, KeywordHandler> _keywordHandlers;
	std::string _statusLine;
	std::map<std::string, std::string> _headers;
	std::vector<char> _tmpBody;
	std::string _body;
	std::string _bodyType;
	StatusCode _statusCode;

	std::map<StatusCode, std::string> _statusMessages;

	// these variables will be extracted from the correct location or from the correct virtual
	// server if needed
	std::string _rootDir;
	bool _autoIndex;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::string _locationUri;
	std::pair<long, std::string> _return;
	bool _allowedMethods[NO_METHOD];
	std::string _cgiExec;

	void initKeywordMap() {
		_keywordHandlers[GET] = &Response::buildGet;
		_keywordHandlers[POST] = &Response::buildPost;
		_keywordHandlers[DELETE] = &Response::buildDelete;
	}

	// function templates
	bool buildGet(RequestParsingResult& request) {
		(void)request;
		return true;
	}

	bool buildPost(RequestParsingResult& request) {
		(void)request;
		return true;
	}

	bool buildDelete(RequestParsingResult& request) {
		(void)request;
		return true;
	}

	bool pushLineToClient(int fd, std::string& line) {
		size_t len = line.length();
		size_t sent = 0;
		int cur_sent;
		while (sent < len) {
			cur_sent = send(fd, line.c_str() + sent, len - sent, 0);
			if (cur_sent == -1) {
				std::cerr << "Error: send failed" << std::endl;
				return false;
			}
			sent += cur_sent;
		}
		return true;
	}

	void buildStatusLine() {
		std::string _statusLine =
			"HTTP/1.1 " + toString(_statusCode) + " " + _statusMessages[_statusCode] + "\r\n";
	}

	void buildHeader() {
		_headers["Date"] = getDate();
		_headers["Server"] = "Webserv/1.0";
		_headers["Content-Length"] = toString(_body.length());
		if (!_bodyType.empty())
			_headers["Content-Type"] = _bodyType;
	}

	void buildErrorPage() {
		std::map<int, std::string>::iterator it = _errorPages.find(_statusCode);
		std::string errorPageUri;
		if (it != _errorPages.end())
			errorPageUri = _rootDir + it->second;
		else
			errorPageUri = _rootDir + _errorPages[DEFAULT_ERROR];
		if (!readHTML(errorPageUri, _body)) {
			_body = "There was an error while trying to access the specified error page for error "
					"code " +
					toString(_statusCode);
			_bodyType = "text/plain";
		} else
			_bodyType = "text/html";
	}

	// for now, only handles html files
	bool buildPage(RequestParsingResult& request) {
		std::string uri = findFinalUri(request.success.uri);
		if (!readHTML(uri, _body)) {
			_statusCode = CLIENT_NOT_FOUND;
			buildErrorPage();
			return false;
		}
		_bodyType = "text/html";
		return true;
	}

	std::string findFinalUri(std::string& uri) {
		int position = comparePrefix(uri, _locationUri);
		return _rootDir + uri.substr(position);
	}

	void buildHeaders(RequestParsingResult& request) { (void)request; }

	// what are the type of things we need to handle to build a response?
	//  1. status line:
	//  	- HTTP version (will always be 1.1)
	//  	- status code (if error when parsing request, use that error code, otherwise check if
	//  the request can be answered, if ok code will be 200)
	//  	- status message (what do we put here?)
	//  2. headers: what are the field we need ?
	//  	- Content-Length (if body is empty, set to 0)
	//  	- Content-Type (if body is empty, set to text/html ?)
	//  	- Date (use current date)
	//  	- Server (use server name)
	//  	- Connection : at first we manade only connection: close, we will see if we handle the
	//  keep-alive mode later
	//  3. body:
	//		- if there is an error code, we need to build a body with the error message based on the
	// correct error page, specific error page or default error page
	//  	- content of the html pages
	//  	- if it is a php page, do we need to convert it to html before sending it ? I think yes
	//  but not sure
	//		- handle differently whether the uri is a directory or a file
	//		- if it is a directory, we need to handle it according to the index and autoindex
	// options

	// TODO: see how to implement to be more efficient and not regenerate for every response
	void initStatusMessageMap() {
		_statusMessages[INFORMATIONAL_CONTINUE] = "Continue";
		_statusMessages[INFORMATIONAL_SWITCHING_PROTOCOLS] = "Switching Protocols";
		_statusMessages[INFORMATIONAL_PROCESSING] = "Processing";
		_statusMessages[INFORMATIONAL_EARLY_HINTS] = "Early Hints";
		_statusMessages[SUCCESS_OK] = "OK";
		_statusMessages[SUCCESS_CREATED] = "Created";
		_statusMessages[SUCCESS_ACCEPTED] = "Accepted";
		_statusMessages[SUCCESS_NON_AUTHORITATIVE_INFORMATION] = "Non-Authoritative Information";
		_statusMessages[SUCCESS_NO_CONTENT] = "No Content";
		_statusMessages[SUCCESS_RESET_CONTENT] = "Reset Content";
		_statusMessages[SUCCESS_PARTIAL_CONTENT] = "Partial Content";
		_statusMessages[SUCCESS_MULTI_STATUS] = "Multi-Status";
		_statusMessages[SUCCESS_ALREADY_REPORTED] = "Already Reported";
		_statusMessages[SUCCESS_IM_USED] = "IM Used";
		_statusMessages[REDIRECTION_MULTIPLE_CHOICES] = "Multiple Choices";
		_statusMessages[REDIRECTION_MOVED_PERMANENTLY] = "Moved Permanently";
		_statusMessages[REDIRECTION_FOUND] = "Found";
		_statusMessages[REDIRECTION_SEE_OTHER] = "See Other";
		_statusMessages[REDIRECTION_NOT_MODIFIED] = "Not Modified";
		_statusMessages[REDIRECTION_USE_PROXY] = "Use Proxy";
		_statusMessages[REDIRECTION_TEMPORARY_REDIRECT] = "Temporary Redirect";
		_statusMessages[REDIRECTION_PERMANENT_REDIRECT] = "Permanent Redirect";
		_statusMessages[CLIENT_BAD_REQUEST] = "Bad Request";
		_statusMessages[CLIENT_UNAUTHORIZED] = "Unauthorized";
		_statusMessages[CLIENT_PAYMENT_REQUIRED] = "Payment Required";
		_statusMessages[CLIENT_FORBIDDEN] = "Forbidden";
		_statusMessages[CLIENT_NOT_FOUND] = "Not Found";
		_statusMessages[CLIENT_METHOD_NOT_ALLOWED] = "Method Not Allowed";
		_statusMessages[CLIENT_NOT_ACCEPTABLE] = "Not Acceptable";
		_statusMessages[CLIENT_PROXY_AUTHENTICATION_REQUIRED] = "Proxy Authentication Required";
		_statusMessages[CLIENT_REQUEST_TIMEOUT] = "Request Timeout";
		_statusMessages[CLIENT_CONFLICT] = "Conflict";
		_statusMessages[CLIENT_GONE] = "Gone";
		_statusMessages[CLIENT_LENGTH_REQUIRED] = "Length Required";
		_statusMessages[CLIENT_PRECONDITION_FAILED] = "Precondition Failed";
		_statusMessages[CLIENT_PAYLOAD_TOO_LARGE] = "Payload Too Large";
		_statusMessages[CLIENT_URI_TOO_LONG] = "URI Too Long";
		_statusMessages[CLIENT_UNSUPPORTED_MEDIA_TYPE] = "Unsupported Media Type";
		_statusMessages[CLIENT_RANGE_NOT_SATISFIABLE] = "Range Not Satisfiable";
		_statusMessages[CLIENT_EXPECTATION_FAILED] = "Expectation Failed";
		_statusMessages[CLIENT_IM_A_TEAPOT] = "I'm a teapot";
		_statusMessages[CLIENT_MISDIRECTED_REQUEST] = "Misdirected Request";
		_statusMessages[CLIENT_UNPROCESSABLE_ENTITY] = "Unprocessable Entity";
		_statusMessages[CLIENT_LOCKED] = "Locked";
		_statusMessages[CLIENT_FAILED_DEPENDENCY] = "Failed Dependency";
		_statusMessages[CLIENT_TOO_EARLY] = "Too Early";
		_statusMessages[CLIENT_UPGRADE_REQUIRED] = "Upgrade Required";
		_statusMessages[CLIENT_PRECONDITION_REQUIRED] = "Precondition Required";
		_statusMessages[CLIENT_TOO_MANY_REQUESTS] = "Too Many Requests";
		_statusMessages[CLIENT_REQUEST_HEADER_FIELDS_TOO_LARGE] = "Request Header Fields Too Large";
		_statusMessages[CLIENT_UNAVAILABLE_FOR_LEGAL_REASONS] = "Unavailable For Legal Reasons";
		_statusMessages[SERVER_INTERNAL_SERVER_ERROR] = "Internal Server Error";
		_statusMessages[SERVER_NOT_IMPLEMENTED] = "Not Implemented";
		_statusMessages[SERVER_BAD_GATEWAY] = "Bad Gateway";
		_statusMessages[SERVER_SERVICE_UNAVAILABLE] = "Service Unavailable";
		_statusMessages[SERVER_GATEWAY_TIMEOUT] = "Gateway Timeout";
		_statusMessages[SERVER_HTTP_VERSION_NOT_SUPPORTED] = "HTTP Version Not Supported";
		_statusMessages[SERVER_VARIANT_ALSO_NEGOTIATES] = "Variant Also Negotiates";
		_statusMessages[SERVER_INSUFFICIENT_STORAGE] = "Insufficient Storage";
		_statusMessages[SERVER_LOOP_DETECTED] = "Loop Detected";
		_statusMessages[SERVER_NOT_EXTENDED] = "Not Extended";
		_statusMessages[SERVER_NETWORK_AUTHENTICATION_REQUIRED] = "Network Authentication Required";
	}
};