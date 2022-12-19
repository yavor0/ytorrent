#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <array>

struct UrlMeta
{
	std::string protocol;
	std::string host;
	std::string port;
	std::string announcePath;
	std::string passKeyParam;
};

std::string urlEncode(const std::string url);
UrlMeta parseTrackerUrl(std::string url);
std::string parseIp(uint32_t ip);

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

// ---------------- Little Endian ----------------
inline uint16_t readAsLE16(const uint8_t *addr)
{
	return (uint16_t)addr[1] << 8 | (uint16_t)addr[0];
}

inline uint32_t readAsLE32(const uint8_t *addr)
{
	return (uint32_t)readAsLE16(addr + 2) << 16 | readAsLE16(addr);
}

inline uint64_t readAsLE64(const uint8_t *addr)
{
	return (uint64_t)readAsLE32(addr + 4) << 32 | readAsLE32(addr);
}

// ---------------- Big Endian ----------------
inline uint16_t readAsBE16(const uint8_t *addr)
{														 // inturpret this function as "take addr and pretend it's a pointer to the start of big endian buffer and then actually perform platform-independent bit shift operations in order to store them".
	return (uint16_t)addr[1] | (uint16_t)(addr[0]) << 8; // maths operations are platform independent
}

inline uint32_t readAsBE32(const uint8_t *addr)
{
	return (uint32_t)readAsBE16(addr + 2) | readAsBE16(addr) << 16;
}

inline uint64_t readAsBE64(const uint8_t *addr)
{
	return (uint64_t)readAsBE32(addr + 4) | (uint64_t)readAsBE32(addr) << 32;
}

#endif
