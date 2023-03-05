#ifndef TRANSCODER_H
#define TRANSCODER_H

#include <stdint.h>
#include <assert.h>
// https://stackoverflow.com/a/2637138/18301773

///////////////// Little Endian
inline uint16_t readAsLE16(const uint8_t *addr)
{
	return (uint16_t)addr[1] << 8 | addr[0];
}

inline uint32_t readAsLE32(const uint8_t *addr)
{
	return (uint32_t)readAsLE16(addr + 2) << 16 | readAsLE16(addr);
}

inline uint64_t readAsLE64(const uint8_t *addr)
{
	return (uint64_t)readAsLE32(addr + 4) << 32 | readAsLE32(addr);
}
inline void writeAsLE16(uint8_t *addr, uint16_t value)
{
	addr[1] = value >> 8;
	addr[0] = (uint8_t)value;
}

inline void writeLE32(uint8_t *addr, uint32_t value)
{
	writeAsLE16(addr + 2, value >> 16);
	writeAsLE16(addr, (uint16_t)value);
}

inline void writeAsLE64(uint8_t *addr, uint64_t value)
{
	writeLE32(addr + 4, value >> 32);
	writeLE32(addr, (uint32_t)value);
}

///////////////// Big Endian
inline uint16_t readAsBE16(const uint8_t *addr)
{														  // inturpret this function as "take addr and pretend it's a pointer to the start of big endian buffer and then actually perform platform-independent bit shift operations in order to store them".
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

inline void writeAsBE16(uint8_t *addr, uint16_t value)
{
	addr[0] = (uint8_t)value >> 8;
	addr[1] = (uint8_t)value;
}

inline void writeAsBE32(uint8_t *addr, uint32_t value)
{
	addr[0] = (uint8_t)(value >> 24);
	addr[1] = (uint8_t)(value >> 16);
	addr[2] = (uint8_t)(value >> 8);
	addr[3] = (uint8_t)(value);
}

inline void writeAsBE64(uint8_t *addr, uint64_t value)
{
	addr[0] = (uint8_t)(value >> 56);
	addr[1] = (uint8_t)(value >> 48);
	addr[2] = (uint8_t)(value >> 40);
	addr[3] = (uint8_t)(value >> 32);
	addr[4] = (uint8_t)(value >> 24);
	addr[5] = (uint8_t)(value >> 16);
	addr[6] = (uint8_t)(value >> 8);
	addr[7] = (uint8_t)(value);
}
#endif
