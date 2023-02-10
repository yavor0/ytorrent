#include "outputmessage.h"
#include <string.h>
#include <utils/transcoder.h>

OutputMessage::OutputMessage(size_t fixedSize)
	: m_pos(0)
{
	if (fixedSize != 0)
		m_buffer.reserve(fixedSize);
}

OutputMessage::~OutputMessage()
{
}

void OutputMessage::addBytes(const uint8_t *bytes, size_t size)
{
	m_buffer.reserve(size);
	memcpy(&m_buffer[m_pos], &bytes[0], size);
	m_pos += size;
}

void OutputMessage::addByte(uint8_t byte)
{
	m_buffer[m_pos++] = byte;
}

void OutputMessage::addU16(uint16_t val)
{
	writeAsBE16(&m_buffer[m_pos], val);
	m_pos += 2;
}

void OutputMessage::addU32(uint32_t val)
{
	writeAsBE32(&m_buffer[m_pos], val);
	m_pos += 4;
}

void OutputMessage::addU64(uint64_t val)
{
	writeAsBE64(&m_buffer[m_pos], val);
	m_pos += 8;
}

void OutputMessage::addString(const std::string &str)
{
	uint16_t len = str.length();

	addU16(len);
	memcpy((char *)&m_buffer[m_pos], str.c_str(), len);
	m_pos += len;
}
