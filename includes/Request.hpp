#pragma once

#include "webserv.hpp"

// TODO return true once a request is finished even if more is in the stack
// TODO one Request for each Client
// TODO fail if no host header field
// TODO illegal characters in request line, header and body
// TODO states for \r\n

// TODO version too long or method too long

// TODO 411 Length Required
// TODO 413 Payload Too Large
// TODO 414 URI Too Long
// TODO 431 Request Header Fields Too Large
// TODO 505 HTTP Version Not Supported

// TODO _statusCode for every return false
// TODO _responseBody for every return false

typedef enum RequestParsingEnum {
	REQUEST_PARSING_CURRENT,
	REQUEST_PARSING_FAILURE,
	REQUEST_PARSING_SUCCESS,
} RequestParsingEnum;

typedef struct RequestParsingSuccess {
	RequestMethod _method;
	std::string _uri;
	std::map<std::string, std::string> _headers;
	std::vector<unsigned char> _body;
} RequestParsingSuccess;

typedef struct RequestParsingFailure {
	StatusCode _statusCode;
	std::string info;
} RequestParsingFailure;

typedef struct RequestParsingResult {
	RequestParsingEnum result;
	RequestParsingSuccess success;
	RequestParsingFailure failure;
} RequestParsingResult;

class Request {
public:
	Request(size_t bufferSize, size_t maxHeaderSize, size_t maxBodySize)
		: _bufferSize(bufferSize), _maxHeaderSize(maxHeaderSize), _maxBodySize(maxBodySize) {
		clear(false);
	}

	RequestParsingResult parse(const unsigned char* s, size_t size) {
		for (size_t i = 0; i < size; ++i)
			_queue.push(s[i]);
		while (!_queue.empty()) {
			unsigned char c = _queue.back();
			_queue.pop();
		}
		return parsingFailure();
	}

private:
	std::queue<unsigned char> _queue;
	std::string line;

	RequestMethod _method;
	std::string _methodString;
	std::string _uri;
	std::string _version;

	std::map<std::string, std::string> _headers;
	std::string _headerName;
	std::string _headerValue;

	std::vector<unsigned char> _body;
	size_t _contentLength;
	bool _isChunked;

	const size_t _bufferSize;
	const size_t _maxHeaderSize;
	const size_t _maxBodySize;

	void clear(bool clearQueue) {
		if (clearQueue)
			std::queue<unsigned char>().swap(_queue);
		_method = NO_METHOD;
		_methodString.clear();
		_uri.clear();
		_version.clear();
		_headers.clear();
		_headerName.clear();
		_headerValue.clear();
		_body.clear();
		_contentLength = 0;
		_isChunked = false;
	}

	RequestParsingResult parsingCurrent() {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_CURRENT;
		// TODO
		return rpr;
	}

	RequestParsingResult parsingFailure() {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_FAILURE;
		// TODO
		clear(true);
		return rpr;
	}

	RequestParsingResult parsingSuccess() {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_SUCCESS;
		// TODO
		clear(false);
		return rpr;
	}
};