#pragma once

#include "webserv.hpp"

class Location {
public:
	Location(VirtualServer* vs);
	~Location();

	bool initUri(std::istringstream& iss);
	bool parseLocationContent(std::istream& config);
	void printLocationInformation() const;

private:
	typedef bool (Location::*KeywordHandler)(std::istringstream& iss);
	typedef enum e_modifier { NONE, REGEX, EXACT } t_modifier;
	VirtualServer* _associatedServer; // TODO const& or const*
	t_modifier _modifier;
	std::string _uri;
	std::string _rootDir;
	bool _autoIndex;
	std::size_t _bufferSize;
	std::size_t _bodySize;
	std::map<int, std::string> _errorPages;
	std::vector<std::string> _indexPages;
	std::map< std::string, KeywordHandler> _keywordHandlers;

	void initKeywordMap();

	bool parseRoot(std::istringstream& iss);
	bool parseAutoIndex(std::istringstream& iss);
	bool parseClientBodyBufferSize(std::istringstream& iss);
	bool parseClientMaxBodySize(std::istringstream& iss);
	bool parseErrorPages(std::istringstream& iss);
	bool parseErrorCode(std::string& code, std::vector<int>& codeList);
	bool parseIndex(std::istringstream& iss);

	void checkIndexPages();
	void checkErrorPages();
};
