#include "inputmessage.h"

#include <utils/transcoder.h>
#include <memory>
#include <string.h>

InputMessage::InputMessage(uint8_t *data, size_t size) :
	m_data(data),
	m_size(size),
	m_pos(0)
{
}

InputMessage::InputMessage() :
	m_data(nullptr),
	m_size(0),
	m_pos(0)
{

}

InputMessage::~InputMessage()
{

}

uint8_t *InputMessage::getBuffer(size_t size)
{
	if (m_pos + size > m_size)
		return nullptr;

	uint8_t *buffer = new uint8_t[size];
	memcpy(buffer, &m_data[m_pos], size);
	return buffer;
}

uint8_t *InputMessage::getBuffer(void)
{
	return &m_data[m_pos];
}

uint8_t InputMessage::getByte()
{
	return m_data[m_pos++];
}

uint16_t InputMessage::getU16()
{
	uint16_t tmp = readAsBE16(&m_data[m_pos]);
	m_pos += 2;
	return tmp;
}

uint32_t InputMessage::getU32()
{
	uint32_t tmp = readAsBE32(&m_data[m_pos]);
	m_pos += 4;
	return tmp;
}

uint64_t InputMessage::getU64()
{
	uint64_t tmp = readAsBE64(&m_data[m_pos]);
	m_pos += 8;
	return tmp;
}

std::string InputMessage::getString()
{
	uint16_t len = getU16();
	if (!len)
		return std::string();

	if (m_pos + len > m_size)
		return std::string();

	std::string ret((char *)&m_data[m_pos], len);
	m_pos += len;
	return ret;
}
