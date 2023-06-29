#pragma once

#include "VirtualServer.hpp"
#include "webserv.hpp"
#include <netinet/in.h>

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

class Request {
public:
	// we consider the first corresponding server to be the default one in case there is an error
	// before finishing parsing the headers
	Request(std::vector<VirtualServer*>& associatedServers, in_addr_t& ip, in_port_t& port)
		: _associatedServers(associatedServers), _ip(ip), _port(port),
		  _maxBodySize(DEFAULT_BODY_SIZE), _matchingServer(associatedServers[0]),
		  _matchingLocation(NULL) {
		clear();
	}

	RequestParsingResult parse(const char* s = NULL, size_t size = 0) {
		for (size_t i = 0; i < size; ++i)
			_queue.push(s[i]);
		while (!_queue.empty()) {
			unsigned char c = _queue.front();
			_queue.pop();
			if (_isInBody) {
				_body.push_back(c);
				if (_body.size() == _contentLength)
					return parsingSuccess();
			} else {
				++_headerSize;
				if (_headerSize > MAX_HEADER_SIZE)
					return parsingFailure(CLIENT_REQUEST_HEADER_FIELDS_TOO_LARGE);
				const bool expectsNewline = !_line.empty() && _line[_line.size() - 1] == '\r';
				if (expectsNewline ? c != '\n' : c != '\r' && !isprint(c)) {
					// std::cout << "here1" << std::endl;
					return parsingFailure(CLIENT_BAD_REQUEST);
				}
				_line += c;
				if (endswith(_line, "\r\n")) {
					_line.resize(_line.size() - 2);
					const StatusCode statusCode = _line.empty()	   ? checkHeaders()
												  : _isRequestLine ? parseRequestLine()
																   : parseHeaderLine();
					if (statusCode != NO_STATUS_CODE)
						return parsingFailure(statusCode);
					if (_line.empty() && _contentLength == 0)
						return parsingSuccess();
					_line.clear();
				}
			}
		}
		return parsingProcessing();
	}

private:
	typedef enum HeaderState {
		HEADER_KEY,
		HEADER_SKIP,
		HEADER_VALUE,
	} HeaderState;

	std::vector<VirtualServer*>& _associatedServers;
	in_addr_t _ip;
	in_port_t _port;
	size_t _maxBodySize;
	VirtualServer* _matchingServer;
	Location* _matchingLocation;

	std::queue<unsigned char> _queue;
	std::string _line;
	size_t _headerSize;
	size_t _contentLength;
	bool _isRequestLine;
	bool _isInBody;

	RequestMethod _method;
	std::string _uri;
	std::map<std::string, std::string> _headers;
	std::vector<unsigned char> _body;

	void clear() {
		std::queue<unsigned char> empty;
		std::swap(_queue, empty);
		_line.clear();
		_headerSize = 0;
		_contentLength = 0;
		_isRequestLine = true;
		_isInBody = false;
		_method = NO_METHOD;
		_uri.clear();
		_headers.clear();
		_body.clear();
	}

	StatusCode parseRequestLine() {
		std::string methodString, version, check;
		std::istringstream iss(_line);
		_isRequestLine = false;
		if (!(iss >> methodString >> _uri >> version) || (iss >> check) || _uri[0] != '/') {
			// std::cout << "here6" << std::endl;
			return CLIENT_BAD_REQUEST;
		}
		if (methodString == "DELETE")
			_method = DELETE;
		else if (methodString == "GET")
			_method = GET;
		else if (methodString == "POST")
			_method = POST;
		else
			return CLIENT_METHOD_NOT_ALLOWED;
		if (_uri.size() > MAX_URI_SIZE)
			return CLIENT_URI_TOO_LONG;
		if (version != "HTTP/1.1")
			return SERVER_HTTP_VERSION_NOT_SUPPORTED;
		// at this point we have a valid request line so we can parse the location in the default
		// server
		findMatchingLocation(_uri);
		return NO_STATUS_CODE;
	}

	StatusCode parseHeaderLine() {
		HeaderState headerState = HEADER_KEY;
		std::string key, value;
		for (size_t i = 0; i < _line.size(); ++i) {
			char c = _line[i];
			switch (headerState) {
			case HEADER_KEY:
				if (c == ':')
					headerState = HEADER_SKIP;
				else
					key += tolower(c);
				break;
			case HEADER_SKIP:
				if (c != ' ') {
					headerState = HEADER_VALUE;
					value += c;
				}
				break;
			case HEADER_VALUE:
				value += c;
				break;
			}
		}
		// std::cout << "key: " << key << std::endl;
		// std::cout << "value: " << value << std::endl;
		if (key.empty() || value.empty()) {
			// std::cout << "here5" << std::endl;
			return CLIENT_BAD_REQUEST;
		}
		// TODO handle duplicates
		_headers[key] = value;
		return NO_STATUS_CODE;
	}

	StatusCode checkHeaders() {
		if (_isRequestLine) {
			// std::cout << "here4" << std::endl;
			return CLIENT_BAD_REQUEST;
		}
		if (_headers.find("host") == _headers.end()) {
			// std::cout << "here3" << std::endl;
			return CLIENT_BAD_REQUEST;
		}
		// at this point we have a host field so we can identify the correct server
		findMatchingServerAndLocation();
		std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
		if (_method == POST) {
			if (it == _headers.end())
				return CLIENT_LENGTH_REQUIRED;
			const std::string contentLengthString = it->second;
			if (contentLengthString.find_first_not_of("0123456789") != std::string::npos) {
				// std::cout << "here2" << std::endl;
				return CLIENT_BAD_REQUEST;
			}
			_contentLength = std::strtol(contentLengthString.c_str(), NULL, 10);
			if (_contentLength > _maxBodySize)
				return CLIENT_PAYLOAD_TOO_LARGE;
		}
		return NO_STATUS_CODE;
	}

	void findMatchingServerAndLocation() {
		std::map<std::string, std::string>::const_iterator it = _headers.find("host");
		if (it == _headers.end()) {
			findBestMatch("");
			findMatchingLocation(_uri);
		} else {
			std::string host = it->second;
			size_t colon = host.find(':');
			if (colon != std::string::npos)
				host.resize(colon);
			findBestMatch(host);
			findMatchingLocation(_uri);
		}
		_maxBodySize = _matchingServer->getBodySize();
	}

	void findBestMatch(const std::string& serverName) {
		int bestMatch = -1;
		VirtualServerMatch bestMatchLevel = VS_MATCH_NONE;
		for (size_t i = 0; i < _associatedServers.size(); ++i) {
			VirtualServerMatch matchLevel =
				_associatedServers[i]->isMatching(_port, _ip, serverName);
			if (matchLevel > bestMatchLevel) {
				bestMatch = i;
				bestMatchLevel = matchLevel;
			}
		}
		_matchingServer = bestMatch == -1 ? NULL : _associatedServers[bestMatch];
	}

	void findMatchingLocation(const std::string& uri) {
		if (_matchingServer == NULL || _matchingServer->getLocations().empty() || uri.empty())
			return;
		_matchingLocation = _matchingServer->findMatchingLocation(uri);
	}

	RequestParsingResult parsingProcessing() {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_PROCESSING;
		rpr.virtualServer = _matchingServer;
		rpr.location = _matchingLocation;
		return rpr;
	}

	RequestParsingResult parsingFailure(StatusCode statusCode) {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_FAILURE;
		rpr.virtualServer = _matchingServer;
		rpr.location = _matchingLocation;
		rpr.statusCode = statusCode;
		clear();
		return rpr;
	}

	RequestParsingResult parsingSuccess() {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_SUCCESS;
		rpr.virtualServer = _matchingServer;
		rpr.location = _matchingLocation;
		rpr.success.method = _method;
		rpr.success.uri = _uri;
		rpr.success.headers = _headers;
		rpr.success.body = _body;
		clear();
		return rpr;
	}
};
