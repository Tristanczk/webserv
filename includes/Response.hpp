#pragma once

#include "webserv.hpp"

class Response {
public:
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