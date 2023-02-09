#include "inputmessage.h"

#include <utils/transcoder.h>
#include <memory>
#include <string.h>

InputMessage::InputMessage(uint8_t *data, size_t size, ByteOrder order) :
	m_data(data),
	m_size(size),
	m_pos(0),
	m_order(order)
{
}

InputMessage::InputMessage(ByteOrder order) :
	m_data(nullptr),
	m_size(0),
	m_pos(0),
	m_order(order)
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

