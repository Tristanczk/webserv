#pragma once

#include "webserv.hpp"

class VirtualServer {
public:
	VirtualServer();
	~VirtualServer();
	VirtualServer& operator=(const VirtualServer& other); // TODO remove if unused

	bool initServer(std::istream& config);
	void printServerInformation(); // TODO delete this function as it uses inet_ntoa

	std::string getRootDir() const;
	bool getAutoIndex() const;
	std::size_t getBufferSize() const;
	std::size_t getBodySize() const;
	std::map<int, std::string> getErrorPages() const; // TODO return const reference?
	std::vector<std::string> getIndexPages() const;	  // TODO return const reference?

private:
	typedef bool (VirtualServer::*KeywordHandler)(std::istringstream& iss);
	struct sockaddr_in _address;
	std::vector<std::string> _serverNames;
	std::string _rootDir;
	bool _autoIndex;
	std::size_t _bufferSize;
	std::size_t _bodySize;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::vector<Location> _locations;
	std::map< std::string, KeywordHandler> _keywordHandlers;

	void initKeywordMap();

	bool parseListen(std::istringstream& iss);
	bool parseServerNames(std::istringstream& iss);
	bool parseRoot(std::istringstream& iss);
	bool parseAutoIndex(std::istringstream& iss);
	bool parseClientBodyBufferSize(std::istringstream& iss);
	bool parseClientMaxBodySize(std::istringstream& iss);
	bool parseErrorPages(std::istringstream& iss);
	bool parseErrorCode(std::string& code, std::vector<int>& codeList);
	bool parseIndex(std::istringstream& iss);
};
