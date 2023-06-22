#pragma once

#include "webserv.hpp"

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
} RequestParsingResult;

class Request {
public:
	Request(size_t maxHeaderSize, size_t maxBodySize)
		: _maxHeaderSize(maxHeaderSize), _maxBodySize(maxBodySize) {
		clear();
	}

	RequestParsingResult parse(const unsigned char* s = NULL, size_t size = 0) {
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
				if (_headerSize > _maxHeaderSize)
					return parsingFailure(CLIENT_REQUEST_HEADER_FIELDS_TOO_LARGE);
				const bool expectsNewline = !_line.empty() && _line[_line.size() - 1] == '\r';
				if (expectsNewline ? c != '\n' : c != '\r' && !isprint(c))
					return parsingFailure(CLIENT_BAD_REQUEST);
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

	const size_t _maxHeaderSize;
	const size_t _maxBodySize;

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
		if (!(iss >> methodString >> _uri >> version) || (iss >> check) || _uri[0] != '/')
			return CLIENT_BAD_REQUEST;
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
					key += c;
				break;
			case HEADER_SKIP:
				if (c != ' ')
					headerState = HEADER_VALUE;
				break;
			case HEADER_VALUE:
				value += c;
				break;
			}
		}
		// il n'y a pas des checks a faire sur la valeur de key ? est-ce qu'il ne faut pas qu'elle
		// prenne un nombre limite de valeurs possibles ? peut-etre pas effectivement, ca fait
		// beaucoup de champs possibles et au pire s'il y en a un mauvais on ne l'utilise juste pas
		// est-ce qu'il ne faut pas verifier que la valeur de key n'est pas deja dans _headers
		// (quelle gestion pour les doublons) ?
		if (key.empty() || value.empty())
			return CLIENT_BAD_REQUEST;
		_headers[key] = value;
		return NO_STATUS_CODE;
	}

	StatusCode checkHeaders() {
		if (_isRequestLine)
			return CLIENT_BAD_REQUEST;
		// pourquoi on verifie qu'il n'y a pas de champ host ? Ca ne devrait pas etre l'inverse ?
		// le nom du champ est Host et pas host non ?
		if (_headers.find("host") != _headers.end())
			return CLIENT_BAD_REQUEST;
		std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
		if (_method == POST) {
			if (it == _headers.end())
				return CLIENT_LENGTH_REQUIRED;
			/// pourquoi on ne check le content-length que pour POST ? il ne faudrait pas le faire
			/// pour les autres methodes s'il y en a un ?
			const std::string contentLengthString = it->second;
			if (contentLengthString.find_first_not_of("0123456789") != std::string::npos)
				return CLIENT_BAD_REQUEST;
			_contentLength = std::strtol(contentLengthString.c_str(), NULL, 10);
			if (_contentLength > _maxBodySize)
				return CLIENT_PAYLOAD_TOO_LARGE;
		}
		return NO_STATUS_CODE;
	}

	RequestParsingResult parsingProcessing() {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_PROCESSING;
		return rpr;
	}

	RequestParsingResult parsingFailure(StatusCode statusCode) {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_FAILURE;
		rpr.statusCode = statusCode;
		clear();
		return rpr;
	}

	RequestParsingResult parsingSuccess() {
		RequestParsingResult rpr;
		rpr.result = REQUEST_PARSING_SUCCESS;
		rpr.success.method = _method;
		rpr.success.uri = _uri;
		rpr.success.headers = _headers;
		rpr.success.body = _body;
		clear();
		return rpr;
	}
};
