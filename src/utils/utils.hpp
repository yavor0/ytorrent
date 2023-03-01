#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include <iomanip>
#include <string>
#include <fstream>
#include <tuple>


enum class ParseResult
{
	SUCCESS,
	CORRUPTED_FILE,
	FAILED_TO_OPEN
};

struct UrlMeta
{
	std::string protocol;
	std::string host;
	std::string port;
	std::string announcePath;
	std::string passKeyParam;
};


UrlMeta parseTrackerUrl(const std::string& url);
std::string urlEncode(const std::string& url);
std::string parseIp(uint32_t ip);
std::string bytesToHumanReadable(uint32_t bytes, bool si);
std::string getcwd();
std::string formatTime(size_t seconds);
bool nodeExists(const std::string& node);

#endif
