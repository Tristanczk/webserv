#pragma once

#include "webserv.hpp"

class Response {
public:
	Response() { initKeywordMap(); }
	~Response(){};

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
		for (std::vector<char>::iterator it = _body.begin(); it != _body.end(); it++) {
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
	std::vector<char> _body;

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

	void buildStatusLine(RequestParsingResult& request) {
		(void)request;
		// if the request is successfully parsed, the status code will depend on the result of the
		// response
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
};