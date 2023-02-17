#include "peer.hpp"
#include "torrent.hpp"
#include <vector>
#include <iostream>

Peer::Peer(Torrent *torrent)
	: torrent(torrent)
{
	conn = new Connection();
	state = AmChoked | PeerChoked;
}

Peer::~Peer()
{
	delete conn;
	for (Piece *p : pieceQueue)
	{
		delete p;
	}
	pieceQueue.clear();
}

void Peer::disconnect()
{
	torrent->removePeer(shared_from_this(), "disconnect called");
	conn->close(false);
}

void Peer::connect(const std::string &ip, const std::string &port)
{
	conn->setErrorCallback(std::bind(&Peer::handleError, shared_from_this(), std::placeholders::_1));
	conn->connect(ip, port,
				  [this]()
				  {
					  const uint8_t *myHandshake = torrent->getHandshake();
					  conn->write(myHandshake, 68);
					  conn->read(68,
								 [this, myHandshake](const uint8_t *peerHandshake, size_t size)
								 {
									 if (size != 68 || (peerHandshake[0] != 0x13 && memcmp(&peerHandshake[1], "BitTorrent protocol", 19) != 0) || memcmp(&peerHandshake[28], &myHandshake[28], 20) != 0)
										 return handleError("info hash/protocol type mismatch");

									 std::string peerId((const char *)&peerHandshake[48], 20);
									 if (!peerId.empty() && peerId != peerId)
										 return handleError("unverified");

									 peerId = peerId;
									 torrent->addPeer(shared_from_this());
									 conn->read(4, std::bind(&Peer::handle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
								 });
				  });
}

void Peer::handle(const uint8_t *data, size_t size)
{
	if (size != 4)
		return handleError("Peer::handle(): Expected 4-byte length");

	uint32_t length = readAsBE32(data);
	switch (length)
	{
	case 0: // Keep alive UNNECESSARY?
		return conn->read(4, std::bind(&Peer::handle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	default:
		conn->read(length,
				   [this](const uint8_t *data, size_t size)
				   {
					   IncomingMessage in(const_cast<uint8_t *>(&data[1]), size - 1);
					   handleMessage((MessageType)data[0], in);
				   });
	}
}

void Peer::handleMessage(MessageType messageType, IncomingMessage in)
{
	size_t payloadSize = in.getSize();

	switch (messageType)
	{
	case CHOKE:
	{
		if (payloadSize != 0)
			return handleError("invalid choke-message size");

		torrent->handlePeerDebug(shared_from_this(), "choke");
		state |= PeerChoked;
		break;
	}
	case UNCHOKE:
	{
		if (payloadSize != 0)
			return handleError("invalid unchoke-message size");

		state &= ~PeerChoked;
		torrent->handlePeerDebug(shared_from_this(), "unchoke");

		for (const Piece *piece : pieceQueue)
		{
			requestPiece(piece->index);
		}

		break;
	}
	case INTERESTED:
	{
		if (payloadSize != 0)
			return handleError("invalid interested-message size");

		torrent->handlePeerDebug(shared_from_this(), "interested");
		state |= PeerInterested;

		if (test_bit(state, AmChoked))
		{
			// 4-byte length, 1-byte packet type
			static const uint8_t unchoke[5] = {0, 0, 0, 1, UNCHOKE};
			conn->write(unchoke, sizeof(unchoke));
			state &= ~AmChoked;
		}

		break;
	}
	case NOT_INTERESTED:
	{
		if (payloadSize != 0)
			return handleError("invalid not-interested-message size");

		torrent->handlePeerDebug(shared_from_this(), "not interested");
		state &= ~PeerInterested;
		break;
	}
	case HAVE:
	{
		if (payloadSize != 4)
			return handleError("invalid have-message size");

		uint32_t p = in.extractNextU32();
		if (!hasPiece(p))
		{
			pieces.push_back(p);
			// torrent->requestPiece(shared_from_this()); // ?????????
		}

		break;
	}
	case BITFIELD:
	{
		if (payloadSize < 1)
			return handleError("invalid bitfield-message size");

		torrent->handlePeerDebug(shared_from_this(), "bit field");
		uint8_t *buf = in.getBuffer(); // std::unique_ptr<uint8_t[]> buf(in.getBuffer());
		for (size_t i = 0, index = 0; i < payloadSize; ++i)
		{
			for (uint8_t x = 128; x > 0; x >>= 1)
			{
				if ((buf[i] & x) != x)
				{
					++index;
					continue;
				}

				if (index >= torrent->getTotalPieces())
					break;

				pieces.push_back(index++);
			}
		}

		torrent->requestPiece(shared_from_this());

		break;
	}
	case REQUEST:
	{
		// bro isn't getting anything from me
		break;
	}
	case PIECE_BLOCK:
	{
		if (payloadSize < 9)
			return handleError("invalid pieceblock-message size");

		uint32_t index, begin;
		index = in.extractNextU32();
		begin = in.extractNextU32();

		payloadSize -= 8; // deduct index and begin
		if (payloadSize <= 0 || payloadSize > maxRequestSize)
			return handleError("received too big piece block of size " + bytesToHumanReadable(payloadSize, true));

		auto it = std::find_if(pieceQueue.begin(), pieceQueue.end(),
							   [index](const Piece *piece)
							   { return piece->index == index; });
		if (it == pieceQueue.end())
			return handleError("received piece " + std::to_string(index) + " which we did not ask for");

		Piece *piece = *it;
		uint32_t blockIndex = begin / maxRequestSize;
		if (blockIndex >= piece->numBlocks)
			return handleError("received too big block index");

		if (torrent->pieceDone(index))
		{
			torrent->handlePeerDebug(shared_from_this(), "cancelling " + std::to_string(index));
			sendCancelRequest(piece);
			pieceQueue.erase(it);
			delete piece;
		}
		else
		{
			piece->blocks[blockIndex].size = payloadSize;
			piece->blocks[blockIndex].data = in.getBuffer(payloadSize);

			if (++piece->currentBlocks == piece->numBlocks)
			{
				std::vector<uint8_t> pieceData;
				pieceData.reserve(piece->numBlocks * maxRequestSize); // just a prediction could be bit less
				for (size_t x = 0; x < piece->numBlocks; ++x)
				{
					for (size_t y = 0; y < piece->blocks[x].size; ++y)
					{
						pieceData.push_back(piece->blocks[x].data[y]);
					}
				}
			}
		}
		break;
	}
	case CANCEL:
	{
		if (payloadSize != 12)
			return handleError("invalid cancel-message size");

		uint32_t index, begin, length;
		index = in.extractNextU32();
		begin = in.extractNextU32();
		length = in.extractNextU32();

		torrent->handlePeerDebug(shared_from_this(), "cancel");
		break;
	}
	}

	conn->read(4, std::bind(&Peer::handle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Peer::handleError(const std::string &errmsg)
{
	torrent->removePeer(shared_from_this(), errmsg);
	conn->close(false); // close but don't call me again
}

void Peer::sendHave(uint32_t index)
{
	if (test_bit(state, AmChoked))
		return;

	OutgoingMessage out(9);
	out.addU32(5UL); // length
	out.addByte(HAVE);
	out.addU32(index);

	conn->write(out);
}

void Peer::sendPieceRequest(uint32_t index)
{
	sendInterested();

	uint32_t pieceLength = torrent->pieceSize(index);
	size_t numBlocks = (int)(ceil(double(pieceLength) / maxRequestSize));

	Piece *piece = new Piece();
	piece->index = index;
	piece->currentBlocks = 0;
	piece->numBlocks = numBlocks;
	piece->blocks = new Block[numBlocks];

	pieceQueue.push_back(piece);
	if (!test_bit(state, PeerChoked))
	{
		requestPiece(index);
	}
}

void Peer::sendRequest(uint32_t index, uint32_t begin, uint32_t length)
{
	OutgoingMessage out(17);
	out.addU32(13UL); // length
	out.addByte(REQUEST);
	out.addU32(index);
	out.addU32(begin);
	out.addU32(length);

	conn->write(out);
}

void Peer::sendInterested()
{
	// 4-byte length, 1 byte packet type
	const uint8_t interested[5] = {0, 0, 0, 1, INTERESTED};
	conn->write(interested, sizeof(interested));
	state |= AmInterested;
}

void Peer::sendCancelRequest(Piece *p)
{
	size_t begin = 0;
	size_t length = torrent->pieceSize(p->index);
	for (; length > maxRequestSize; length -= maxRequestSize, begin += maxRequestSize)
	{
		sendCancel(p->index, begin, maxRequestSize);
	}

	sendCancel(p->index, begin, length);
}

void Peer::sendCancel(uint32_t index, uint32_t begin, uint32_t length)
{
	OutgoingMessage out(17);
	out.addU32(13UL); // length
	out.addByte(CANCEL);
	out.addU32(index);
	out.addU32(begin);
	out.addU32(length);

	conn->write(out);
}

void Peer::requestPiece(size_t pieceIndex)
{
	if (test_bit(state, PeerChoked))
		return handleError("Attempt to request piece from a peer that is remotely choked");

	size_t begin = 0;
	size_t length = torrent->pieceSize(pieceIndex);
	for (; length > maxRequestSize; length -= maxRequestSize, begin += maxRequestSize) // rewrite with a while for clarity
	{
		sendRequest(pieceIndex, begin, maxRequestSize);
	}

	sendRequest(pieceIndex, begin, length);
}
