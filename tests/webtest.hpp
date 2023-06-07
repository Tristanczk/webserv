#pragma once

#include "../includes/webserv.hpp"

#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

#define RESET "\033[0m"
#define BOLDBLUE "\033[1m\033[34m"
#define RED "\033[31m"
#define GREEN "\033[32m"

#define VALID_DIRECTORY "conf/valid/"
#define INVALID_DIRECTORY "conf/invalid/"
#define CONF_NOT_FOUND "conf/invalid/notfound.conf"
#define CONF_UNREADABLE "conf/invalid/unreadable.conf"

void displayTitle(const std::string&);
void displayResult(const std::string&, bool);
void testParseConfig();
