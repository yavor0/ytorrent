#include "peer.hpp"
#include "torrent.hpp"
#include <vector>
#include <iostream>

Peer::Peer(Torrent *t)
	: torrent(t),
	  state(4)
{
	conn = std::make_shared<Connection>(); // https://stackoverflow.com/a/5558955/18301773
	state.set(AM_CHOKING);
	state.set(PEER_CHOKING);
}

Peer::Peer(Torrent *t, const std::shared_ptr<Connection> &conn)
	: torrent(t),
	  conn(conn),
	  state(4)
{
	state.set(AM_CHOKING);
	state.set(PEER_CHOKING);
}

Peer::~Peer()
{
	// std::clog << "\n\n\n\n Destructor CALLED \n\n\n\n" << std::endl;
	for (size_t i = 0; i < pieceQueue.size(); i++)
	{
		delete pieceQueue[i];
	}
}

void Peer::disconnect()
{
	conn->close();
	conn->setErrorCallback(nullptr);
}

void Peer::connect(const std::string &ip, const std::string &port)
{
	conn->setErrorCallback(std::bind(&Peer::handleError, shared_from_this(), std::placeholders::_1));
	conn->connect(ip, port,
				  [this]()
				  {
					  const uint8_t *myHandshake = (this->torrent)->getHandshake();
					  (this->conn)->write(myHandshake, 68);
					  (this->conn)->read(68, [this, myHandshake](const uint8_t *peerHandshake, size_t size)
										 {
										   if (size != 68 || (peerHandshake[0] != 19 && memcmp(&peerHandshake[1], "BitTorrent protocol", 19) != 0) || memcmp(&peerHandshake[28], &myHandshake[28], 20) != 0)
										   {
												this->handleError("info hash/protocol type mismatch");
												return;
										   }

										   std::string peerId((const char *)&peerHandshake[48], 20);
										   this->peerId = peerId;

										   (this->torrent)->handshaked(this->shared_from_this());
										   (this->conn)->read(4, std::bind(&Peer::handle, this, std::placeholders::_1, std::placeholders::_2)); });
				  });
}

void Peer::authenticate()
{
	const uint8_t *myHandshake = torrent->getHandshake();
	conn->setErrorCallback(std::bind(&Peer::handleError, shared_from_this(), std::placeholders::_1));
	conn->read(68,
			   [this, myHandshake](const uint8_t *peerHandshake, size_t size)
			   {
				   if (size != 68 || (peerHandshake[0] != 0x13 && memcmp(&peerHandshake[1], "BitTorrent protocol", 19) != 0) || memcmp(&myHandshake[28], &peerHandshake[28], 20) != 0)
				   {
					   handleError("info hash/protocol type mismatch");
					   return;
				   }

				   std::string peerId((const char *)&peerHandshake[48], 20);
				   this->peerId = peerId;
				   (this->conn)->write(myHandshake, 68);
				   (this->torrent)->handshaked(this->shared_from_this());

				   this->sendBitfield((this->torrent)->getRawBitfield());

				   (this->torrent)->handlePeerDebug(shared_from_this(), "connected! (" + std::to_string((this->torrent)->getActivePeers()) + " established)");
				   (this->conn)->read(4, std::bind(&Peer::handle, this, std::placeholders::_1, std::placeholders::_2));
			   });
}

void Peer::handle(const uint8_t *data, size_t size)
{
	if (size != 4)
		return handleError("Peer::handle(): Expected 4-byte length");

	uint32_t length = readAsBE32(data);
	switch (length)
	{
	case 0:
		return conn->read(4, std::bind(&Peer::handle, this, std::placeholders::_1, std::placeholders::_2));
	default:
		conn->read(length,
				   [this](const uint8_t *data, size_t size)
				   {
					   IncomingMessage in(const_cast<uint8_t *>(&data[1]), size - 1);
					   this->handleMessage((MessageID)data[0], in);
				   });
	}
}

void Peer::handleMessage(MessageID messageID, IncomingMessage inMsg)
{
	size_t msgSize = inMsg.getSize();

	switch (messageID)
	{
	case CHOKE:
	{
		if (msgSize != 0)
			return handleError("invalid choke-message size");

		torrent->handlePeerDebug(shared_from_this(), "choke");
		state.set(PEER_CHOKING);
		break;
	}
	case UNCHOKE:
	{
		if (msgSize != 0)
			return handleError("invalid unchoke-message size");

		state.reset(PEER_CHOKING);
		torrent->handlePeerDebug(shared_from_this(), "unchoke");

		for (const Piece *piece : pieceQueue)
		{
			if (piece != nullptr)
			{
				requestPiece(piece->index);
			}
		}

		break;
	}
	case INTERESTED:
	{
		if (msgSize != 0)
			return handleError("invalid interested-message size");

		torrent->handlePeerDebug(shared_from_this(), "interested");
		state.set(PEER_INTERESTED);

		if (state.test(AM_CHOKING)) // literally just ask bro
		{
			// 4-byte length, 1-byte message type
			static const uint8_t unchoke[5] = {0, 0, 0, 1, UNCHOKE}; // just make it big-endian here, dont bother creating OutgoingMessage
			conn->write(unchoke, sizeof(unchoke));
			state.reset(AM_CHOKING);
		}

		break;
	}
	case NOT_INTERESTED:
	{
		if (msgSize != 0)
			return handleError("invalid not-interested-message size");

		torrent->handlePeerDebug(shared_from_this(), "not interested");
		state.reset(PEER_INTERESTED);
		break;
	}
	case HAVE:
	{
		if (msgSize != 4)
			return handleError("invalid have-message size");

		uint32_t p = inMsg.extractNextU32();
		if (!hasPiece(p))
		{
			hasPieceIndexes.push_back(p);
		}

		break;
	}
	case BITFIELD:
	{
		if (msgSize < 1)
			return handleError("invalid bitfield-message size");

		torrent->handlePeerDebug(shared_from_this(), "bit field");
		uint8_t *raw = inMsg.getBuffer();
		for (size_t i = 0; i < msgSize; ++i)
		{
			for (uint8_t j = 0; j < 8; ++j)
			{
				if (raw[i] & (1 << (7 - j)))
				{
					size_t index = i * 8 + j;
					if (index < torrent->getTotalPieces())
					{
						hasPieceIndexes.push_back(index);
					}
				}
			}
		}

		if (!torrent->isFullyDownloaded())
		{
			torrent->selectPieceAndRequest(shared_from_this()); // why shared_from_this
		}
		break;
	}
	case REQUEST:
	{
		if (msgSize != 12)
			return handleError("invalid request-message size");

		if (!state.test(PEER_INTERESTED))
			return handleError("peer requested piece block without showing interest");

		if (state.test(AM_CHOKING))
			return handleError("peer requested piece while choked");

		uint32_t index, begin, length;
		index = inMsg.extractNextU32();
		begin = inMsg.extractNextU32();
		length = inMsg.extractNextU32();

		if (length > MAX_BLOCK_REQUEST_SIZE)
			return handleError("peer requested piece of size " + bytesToHumanReadable(length, true) + " which is beyond our max request size");

		if (!torrent->pieceDone(index))
		{
			break;
		}

		torrent->handlePeerDebug(shared_from_this(), "requested piece block of length " + bytesToHumanReadable(length, true));
		torrent->handleBlockRequest(shared_from_this(), index, begin, length);
		break;
	}
	case PIECE_BLOCK:
	{
		if (msgSize < 9)
			return handleError("invalid pieceblock-message size");

		uint32_t index, begin;
		index = inMsg.extractNextU32();
		begin = inMsg.extractNextU32();

		msgSize -= 8; // deduct index and begin
		if (msgSize <= 0 || msgSize > MAX_BLOCK_REQUEST_SIZE)
			return handleError("received too big piece block of size " + bytesToHumanReadable(msgSize, true));

		auto it = std::find_if(pieceQueue.begin(), pieceQueue.end(),
							   [index](const Piece *piece)
							   { return piece->index == index; });
		if (it == pieceQueue.end())
			return handleError("received piece " + std::to_string(index) + " which wasn't asked for");

		Piece *piece = *it;
		uint32_t blockIndex = begin / MAX_BLOCK_REQUEST_SIZE;
		if (blockIndex >= piece->totalBlocks)
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
			piece->blocks[blockIndex].size = msgSize;
			piece->blocks[blockIndex].data = inMsg.cpyBuffer(msgSize);
			piece->haveBlocks++;

			if (piece->haveBlocks == piece->totalBlocks)
			{
				std::vector<uint8_t> pieceData;
				pieceData.reserve(piece->totalBlocks * MAX_BLOCK_REQUEST_SIZE); // just a prediction could be less but not more
				for (size_t i = 0; i < piece->totalBlocks; i++)
				{
					for (size_t j = 0; j < piece->blocks[i].size; j++)
					{
						pieceData.push_back(piece->blocks[i].data[j]);
					}
				}

				torrent->handlePieceCompleted(shared_from_this(), index, pieceData);
				pieceQueue.erase(it);
				delete piece; // explodes if i dont delete the piece??

				// Have to do this here, if its done inside of handlePieceCompleted
				// pieceQueue will fail due to sendPieceRequest changing position
				if (torrent->getCompletedPieces() != torrent->getTotalPieces())
				{
					torrent->selectPieceAndRequest(shared_from_this());
				}
			}
		}
		break;
	}
	case CANCEL:
	{
		if (msgSize != 12)
			return handleError("invalid cancel-message size");

		uint32_t index, begin, length;
		index = inMsg.extractNextU32();
		begin = inMsg.extractNextU32();
		length = inMsg.extractNextU32();

		torrent->handlePeerDebug(shared_from_this(), "cancel");
		break;
	}
	}
	conn->read(4, std::bind(&Peer::handle, this, std::placeholders::_1, std::placeholders::_2));
}

void Peer::handleError(const std::string &errmsg)
{
	// ORDER IS VERY IMPORTANT!!!!
	torrent->removePeer(shared_from_this(), errmsg);
	std::lock_guard<std::mutex> guard(this->torrent->peerContainersMutex);
	disconnect();
}

void Peer::sendHave(uint32_t index)
{
	if (state.test(AM_CHOKING))
		return;

	OutgoingMessage out(9);
	out.addU32(5UL); // length
	out.addU8(HAVE);
	out.addU32(index);

	conn->write(out);
}

void Peer::sendBitfield(std::vector<uint8_t> rBitfield)
{
	// std::clog << "\n\n\n" << rBitfield.size() << "\n\n\n" << std::endl;
	OutgoingMessage out(5 + rBitfield.size());
	out.addU32(1UL + rBitfield.size()); // length
	out.addU8(BITFIELD);
	out.addCustom(rBitfield.data(), rBitfield.size()); // already in big endian

	conn->write(out);
}

void Peer::sendPieceRequest(uint32_t index)
{
	sendInterested();
	uint32_t pieceLength = torrent->pieceSize(index);
	size_t numBlocks = (size_t)(ceil(double(pieceLength) / MAX_BLOCK_REQUEST_SIZE)); // https://wiki.theory.org/BitTorrentSpecification#Notes

	Piece *piece = new Piece();
	piece->index = index;
	piece->haveBlocks = 0;
	piece->totalBlocks = numBlocks;
	piece->blocks = new Block[numBlocks];

	pieceQueue.push_back(piece);
	if (!state.test(PEER_CHOKING))
	{
		requestPiece(index);
	}
}

void Peer::sendPieceBlock(uint32_t index, uint32_t begin, uint8_t *block, uint32_t length)
{
	OutgoingMessage out(13 + length);
	out.addU32(9UL + length); // length
	out.addU8((uint8_t)PIECE_BLOCK);
	out.addU32(index);
	out.addU32(begin);
	out.addCustom(block, length); // already in be

	conn->write(out);
}

void Peer::sendRequest(uint32_t index, uint32_t begin, uint32_t length)
{
	OutgoingMessage out(17);
	out.addU32(13UL); // length
	out.addU8(REQUEST);
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
	state.set(AM_INTERESTED);
}

void Peer::sendCancelRequest(Piece *p)
{
	size_t begin = 0;
	size_t length = torrent->pieceSize(p->index);
	while (length > MAX_BLOCK_REQUEST_SIZE)
	{
		sendCancel(p->index, begin, MAX_BLOCK_REQUEST_SIZE);
		length -= MAX_BLOCK_REQUEST_SIZE;
		begin += MAX_BLOCK_REQUEST_SIZE;
	}
	sendCancel(p->index, begin, length);
}

void Peer::sendCancel(uint32_t index, uint32_t begin, uint32_t length)
{
	OutgoingMessage out(17);
	out.addU32(13UL); // length
	out.addU8(CANCEL);
	out.addU32(index);
	out.addU32(begin);
	out.addU32(length);

	conn->write(out);
}

void Peer::requestPiece(size_t pieceIndex)
{
	if (state.test(PEER_CHOKING))
	{
		return handleError("Attempt to request piece from a peer that is remotely choked");
	}
	size_t begin = 0;
	size_t length = torrent->pieceSize(pieceIndex);

	while (length > MAX_BLOCK_REQUEST_SIZE)
	{
		sendRequest(pieceIndex, begin, MAX_BLOCK_REQUEST_SIZE);
		length -= MAX_BLOCK_REQUEST_SIZE;
		begin += MAX_BLOCK_REQUEST_SIZE;
	}
	sendRequest(pieceIndex, begin, length);
}
