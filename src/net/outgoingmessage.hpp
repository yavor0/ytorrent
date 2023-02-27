#ifndef OUTGOING_MESSAGE_H
#define OUTGOING_MESSAGE_H

#include <utils/transcoder.hpp>
#include <vector>

class OutgoingMessage
{
private:
	std::vector<uint8_t> dataBuffer;
	size_t writeIndex;

public:
	OutgoingMessage(size_t fixedSize = 0) : writeIndex(0)
	{
		if (fixedSize != 0)
			dataBuffer.reserve(fixedSize);
	}

	~OutgoingMessage()
	{
	}

	void clear()
	{
		dataBuffer.clear();
		writeIndex = 0;
	}
	void addCustom(const uint8_t *bytes, size_t size)
	{
		dataBuffer.reserve(size);
		memcpy(&dataBuffer[writeIndex], &bytes[0], size);
		writeIndex += size;
	}

	void addU8(uint8_t byte)
	{
		dataBuffer[writeIndex++] = byte;
	}
	void addU16(uint16_t val)
	{
		writeAsBE16(&dataBuffer[writeIndex], val);
		writeIndex += 2;
	}
	void addU32(uint32_t val)
	{
		writeAsBE32(&dataBuffer[writeIndex], val);
		writeIndex += 4;
	}
	void addU64(uint64_t val)
	{
		writeAsBE64(&dataBuffer[writeIndex], val);
		writeIndex += 8;
	}

	const uint8_t *data() const { return &dataBuffer[writeIndex]; }
	const uint8_t *data(size_t p) const { return &dataBuffer[p]; }
	size_t size() const { return writeIndex; }
};

#endif