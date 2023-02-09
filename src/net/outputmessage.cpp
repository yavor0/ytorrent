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


void OutputMessage::addString(const std::string &str)
{
	uint16_t len = str.length();

	addU16(len);
	memcpy((char *)&m_buffer[m_pos], str.c_str(), len);
	m_pos += len;
}
