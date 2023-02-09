#ifndef INPUTMESSAGE_H
#define INPUTMESSAGE_H

#include "decl.h"

class InputMessage
{
public:
	InputMessage(uint8_t *data, size_t size, ByteOrder order);
	InputMessage(ByteOrder order);
	~InputMessage();

	void setData(uint8_t *d) { m_data = d; }
	size_t getSize() { return m_size; }
	void setSize(size_t size) { m_size = size; }
	void setByteOrder(ByteOrder order) { m_order = order; }

	uint8_t *getBuffer(size_t size);
	uint8_t *getBuffer(void);
	uint8_t getByte();
	uint16_t getU16();


	inline InputMessage &operator=(uint8_t *data)
	{
		m_data = data;
		return *this;
	}
	inline InputMessage &operator>>(uint8_t &b)
	{
		b = getByte();
		return *this;
	}
	inline InputMessage &operator>>(uint16_t &u)
	{
		u = getU16();
		return *this;
	}
	inline InputMessage &operator>>(uint32_t &u)
	{
		u = getU32();
		return *this;
	}
	inline InputMessage &operator>>(uint64_t &u)
	{
		u = getU64();
		return *this;
	}
	inline InputMessage &operator>>(std::string &s)
	{
		s = getString();
		return *this;
	}

private:
	uint8_t *m_data;
	size_t m_size;
	uint32_t m_pos;
	ByteOrder m_order;
};

#endif

