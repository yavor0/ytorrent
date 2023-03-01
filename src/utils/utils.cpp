#include "utils.hpp"
#include <string.h>

#include <regex>
#include <memory>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <iostream>
#include <stdexcept>

// http://stackoverflow.com/a/7214192/502230
std::string urlEncode(const std::string &value)
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

UrlMeta parseTrackerUrl(const std::string &url)
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

	return urlMeta;
}

// http://stackoverflow.com/a/3758880/1551592
std::string bytesToHumanReadable(uint32_t bytes, bool si)
{
	uint32_t u = si ? 1000 : 1024;
	if (bytes < u)
		return std::to_string(bytes) + " B";

	size_t exp = static_cast<size_t>(std::log(bytes) / std::log(u));
	const char *e = si ? "kMGTPE" : "KMGTPE";
	std::ostringstream os;
	os << std::fixed << std::showpoint << std::setprecision(2) << static_cast<double>(bytes / std::pow(u, exp)) << " ";
	os << e[exp - 1] << (si ? "" : "i") << "B";

	return os.str();
}

// https://stackoverflow.com/a/2869667/18301773
std::string getcwd()
{
	const size_t chunkSize = 255;
	const int maxChunks = 10240; // 2550 KiBs of current path are more than enough

	char stackBuffer[chunkSize]; // Stack buffer for the "normal" case
	if (getcwd(stackBuffer, sizeof(stackBuffer)) != NULL)
		return stackBuffer;
	if (errno != ERANGE)
	{
		// It's not ERANGE, so we don't know how to handle it
		throw std::runtime_error("Cannot determine the current path.");
		// Of course you may choose a different error reporting method
	}
	// Ok, the stack buffer isn't long enough; fallback to heap allocation
	for (int chunks = 2; chunks < maxChunks; chunks++)
	{
		// With boost use scoped_ptr; in C++0x, use unique_ptr
		// If you want to be less C++ but more efficient you may want to use realloc
		std::unique_ptr<char> cwd(new char[chunkSize * chunks]);
		if (getcwd(cwd.get(), chunkSize * chunks) != NULL)
			return cwd.get();
		if (errno != ERANGE)
		{
			// It's not ERANGE, so we don't know how to handle it
			throw std::runtime_error("Cannot determine the current path.");
			// Of course you may choose a different error reporting method
		}
	}
	throw std::runtime_error("Cannot determine the current path; the path is apparently unreasonably long");
}

std::string formatTime(size_t seconds)
{
    size_t minutes = seconds / 60;
    size_t remainingSeconds = seconds % 60;
    std::string minutesStr = std::to_string(minutes);
    std::string secondsStr = std::to_string(remainingSeconds);
    if (minutes < 10)
    {
        minutesStr = "0" + minutesStr;
    }
    if (remainingSeconds < 10)
    {
        secondsStr = "0" + secondsStr;
    }
    return minutesStr + ":" + secondsStr;
}
bool nodeExists(const std::string &node)
{
	struct stat st;
	return stat(node.c_str(), &st) == 0;
}
