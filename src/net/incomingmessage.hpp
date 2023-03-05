#ifndef INCOMING_MESSAGE_H
#define INCOMING_MESSAGE_H

#include <utils/transcoder.hpp>
#include <string>
#include <memory>
#include <cstring>

class IncomingMessage
{
private:
	uint8_t *dataBuffer;
	size_t buffSize;
	uint32_t readIndex;

public:
	IncomingMessage(uint8_t *data, size_t size) : dataBuffer(data),
												  buffSize(size),
												  readIndex(0)
	{
	}
	IncomingMessage() : dataBuffer(nullptr),
						buffSize(0),
						readIndex(0)
	{
	}
	~IncomingMessage()
	{
	}

	void setData(uint8_t *d) { dataBuffer = d; }
	size_t getSize() { return buffSize; }
	void setSize(size_t size) { buffSize = size; }

	uint8_t *cpyBuffer(size_t size)
	{
		if (readIndex + size > buffSize)
			return nullptr;

		uint8_t *buffer = new uint8_t[size];
		memcpy(buffer, &dataBuffer[readIndex], size);
		return buffer;
	}

	uint8_t *getBuffer()
	{
		return &dataBuffer[readIndex];
	}

	uint8_t extractNextByte()
	{
		return dataBuffer[readIndex++];
	}
	uint16_t extractNextU16()
	{
		uint16_t tmp = readAsBE16(&dataBuffer[readIndex]);
		readIndex += 2;
		return tmp;
	}

	uint32_t extractNextU32()
	{
		uint32_t tmp = readAsBE32(&dataBuffer[readIndex]);
		readIndex += 4;
		return tmp;
	}
	uint64_t extractNextU64()
	{
		uint64_t tmp = readAsBE64(&dataBuffer[readIndex]);
		readIndex += 8;
		return tmp;
	}
};
#endif
