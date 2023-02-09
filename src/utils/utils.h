#ifndef UTILS_H
#define UTILS_H


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


#endif
