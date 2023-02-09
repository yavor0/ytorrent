#ifndef OUTPUTMESSAGE_H
#define OUTPUTMESSAGE_H

#include "decl.h"

class OutputMessage
{
public:
	OutputMessage(size_t fixedSize = 0);
	~OutputMessage();

	void clear() { m_buffer.clear(); m_pos = 0; }
	void addByte(uint8_t byte);
	void addBytes(const uint8_t *bytes, size_t size);

	inline OutputMessage &operator<<(const uint8_t &b)
	{
		addByte(b);
		return *this;
	}
	inline OutputMessage &operator<<(const uint16_t &u)
	{
		addU16(u);
		return *this;
	}
	inline OutputMessage &operator<<(const uint32_t &u)
	{
		addU32(u);
		return *this;
	}
	inline OutputMessage &operator<<(const uint64_t &u)
	{
		addU64(u);
		return *this;
	}
	inline OutputMessage &operator<<(const std::string &s)
	{
		addString(s);
		return *this;
	}

private:
	DataBuffer<uint8_t> m_buffer;
	size_t m_pos;
};

#endif
