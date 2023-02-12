#ifndef OUTPUTMESSAGE_H
#define OUTPUTMESSAGE_H

#include <utils/transcoder.h>
#include <vector>
#include <string>

class OutputMessage
{
public:
	OutputMessage(size_t fixedSize = 0);
	~OutputMessage();

	void clear() { m_buffer.clear(); m_pos = 0; }
	void addByte(uint8_t byte);
	void addBytes(const uint8_t *bytes, size_t size);
	void addU16(uint16_t val);
	void addU32(uint32_t val);
	void addU64(uint64_t val);
	void addString(const std::string &str);

	const uint8_t *data() const { return &m_buffer[m_pos]; }
	const uint8_t *data(size_t p) const { return &m_buffer[p]; }
	size_t size() const { return m_pos; }

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
	std::vector<uint8_t> m_buffer;
	size_t m_pos;
};

#endif
