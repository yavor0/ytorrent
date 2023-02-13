#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <fstream>
#include <tuple>


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

bool nodeExists(const std::string& node);

static inline bool test_bit(uint32_t bits, uint32_t bit)
{
	return (bits & bit) == bit;
}

#endif
