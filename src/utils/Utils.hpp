#ifndef UTIL_H
#define UTIL_H
#include <string>
#include <array>

struct UrlMeta
{
    std::string protocol;
    std::string host;
    std::string port;
};

std::string urlEncode(const std::string url);
UrlMeta parseUrl(std::string url);


template <typename T, size_t N>
std::array<unsigned char, N * sizeof(T)> getBytesEndianIndependent(const T (&arrRef)[N])
{
	// https://en.cppreference.com/w/cpp/named_req/StandardLayoutType
	static_assert(std::is_standard_layout<T>::value, "Standard layout type expected");

	std::array<unsigned char, N * sizeof(T)> result;

#if __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__

	size_t count = 0;
	const unsigned char *p = nullptr;
	for (const T &elem : arrRef)
	{
		p = (const unsigned char *)&elem;
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			result[count++] = p[sizeof(T) - i - 1];
		}
	}
#elif __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__
	std::memcpy(&result, &arrRef, N * sizeof(T));
#else
#error "Unknown endianness detected"
#endif
	return result;
}

#endif
