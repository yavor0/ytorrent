#include "Utils.hpp"
#include <iomanip>
#include <sstream>
#include <regex>

#include <iostream>

// http://stackoverflow.com/a/7214192/502230
std::string urlEncode(const std::string value)
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

UrlMeta parseUrl(std::string url)
{
	UrlMeta urlMeta;
	std::regex httpOrHttpsWithPort("^(https?://)([^:/]+)(:\\d+)(/ann.*)");
	std::regex httpOrHttpsWithoutPort("^(https?://)([^:/]+)(/ann.*)");
	std::smatch match;
	if (std::regex_match(url, match, httpOrHttpsWithPort))
	{
		urlMeta.protocol = match[1];
		urlMeta.host = match[2];
		urlMeta.port = match[3];
		urlMeta.port.erase(0, 1);
	}
	else if (std::regex_match(url, match, httpOrHttpsWithoutPort))
	{
		urlMeta.protocol = match[1];
		urlMeta.host = match[2];
		// urlMeta.port = (urlMeta.protocol == std::string("http")) ? "80" : "443";
		urlMeta.port = "80";
	}
	else
	{
		std::cout << "No match found" << std::endl;
	}

	return urlMeta;
}


