#include "utils.h"
#include <string.h>

#include <regex>
#include <memory>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <iostream>
#include <stdexcept>

// http://stackoverflow.com/a/7214192/502230
std::string urlEncode(const std::string& value)
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex << std::uppercase;

	for (std::string::const_iterator i = value.begin(); i != value.end(); ++i)
	{
		std::string::value_type c = *i;
		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			escaped << c;
		else // Any other characters are percent-encoded
			escaped << '%' << std::setw(2) << static_cast<int>((unsigned char)c);
	}

	return escaped.str();
}

std::string parseIp(uint32_t ip)
{
	std::ostringstream formatted;
	uint8_t byte3 = (uint8_t)((ip & 0xFF000000) >> 24);
	uint8_t byte2 = (uint8_t)((ip & 0x00FF0000) >> 16);
	uint8_t byte1 = (uint8_t)((ip & 0x0000FF00) >> 8);
	uint8_t byte0 = (uint8_t)(ip & 0x000000FF);

	// really ugly but does the trick
	// basically: https://stackoverflow.com/a/31991844/18301773
	// and: https://en.cppreference.com/w/cpp/language/implicit_conversion#Integral_promotion
	formatted << 0 + byte0 << "." << 0 + byte1 << "." << 0 + byte2 << "." << 0 + byte3;
	return formatted.str();
}

UrlMeta parseTrackerUrl(const std::string& url)
{
	UrlMeta urlMeta;
	std::regex httpOrHttpsWithPort("^(https?://)([^:/]+)(:\\d+)(/ann[^\\?]*)(\\?passkey=.*)?");
	std::regex httpOrHttpsWithoutPort("^(https?://)([^:/]+)(/ann[^\\?]*)(\\?passkey=.*)?");
	std::smatch match;
	if (std::regex_match(url, match, httpOrHttpsWithPort))
	{
		urlMeta.protocol = match[1];
		urlMeta.host = match[2];
		urlMeta.port = match[3];
		urlMeta.port.erase(0, 1);
		urlMeta.announcePath = match[4];
		if (match.size() == 6)
		{
			urlMeta.passKeyParam = match[5];
			std::replace(urlMeta.passKeyParam.begin(), urlMeta.passKeyParam.end(), '?', '&');
		}
	}
	else if (std::regex_match(url, match, httpOrHttpsWithoutPort))
	{
		urlMeta.protocol = match[1];
		urlMeta.host = match[2];
		// urlMeta.port = (urlMeta.protocol == std::string("http")) ? "80" : "443";
		urlMeta.port = "80";
		urlMeta.announcePath = match[3];
		if (match.size() == 5)
		{
			urlMeta.passKeyParam = match[4];
			std::replace(urlMeta.passKeyParam.begin(), urlMeta.passKeyParam.end(), '?', '&');
		}
	}
	else
	{
		std::cout << "No match found" << std::endl;
	}

	return urlMeta;
}

