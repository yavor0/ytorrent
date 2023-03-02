#ifndef OUTGOING_MESSAGE_H
#define OUTGOING_MESSAGE_H

#include <utils/transcoder.hpp>
#include <vector>

class OutgoingMessage
{
private:
	std::vector<uint8_t> dataBuffer;

public:
	OutgoingMessage(size_t fixedSize)
	{
		// to reduce heap allocations for small byte insertions
		dataBuffer.reserve(fixedSize);
	}

	~OutgoingMessage()
	{
	}

	void addCustom(const uint8_t *bytes, size_t size)
	{
		dataBuffer.insert(dataBuffer.end(), &bytes[0], &bytes[size]);
	}

	void addU8(uint8_t byte)
	{
		dataBuffer.push_back(byte);
	}
	void addU16(uint16_t val)
	{
		uint8_t bytes[sizeof(val)];

		writeAsBE16(&bytes[0], val);
		addCustom(&bytes[0], sizeof(val));
	}
	void addU32(uint32_t val)
	{
		uint8_t bytes[sizeof(val)];

		writeAsBE32(&bytes[0], val);
		addCustom(&bytes[0], sizeof(val));
	}
	void addU64(uint64_t val)
	{
		uint8_t bytes[sizeof(val)];

		writeAsBE64(&bytes[0], val);
		addCustom(&bytes[0], sizeof(val));
	}
	const uint8_t *data() const { return &dataBuffer.at(0); }
	size_t size() const { return dataBuffer.size(); }
};

#endif