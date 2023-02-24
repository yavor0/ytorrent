#ifndef PEER_H
#define PEER_H

#include <net/connection.hpp>
#include <net/incomingmessage.hpp>

#include <memory>
#include <vector>

class Torrent;
class Peer : public std::enable_shared_from_this<Peer>
{
	struct Piece;
	enum State : uint8_t
	{
		AM_CHOKING = 1 << 0,		 // I choked this peer (i.e. we're not giving him anymore pieces)
		AM_INTERESTED = 1 << 1,	 // I'm interested in this peer's pieces
		PEER_CHOKED = 1 << 2,	 // Peer choked me
		PEER_INTERESTED = 1 << 3, // Peer interested in my stuff
	};

	enum MessageType : uint8_t
	{
		CHOKE = 0,
		UNCHOKE = 1,
		INTERESTED = 2,
		NOT_INTERESTED = 3,
		HAVE = 4,
		BITFIELD = 5,
		REQUEST = 6,
		PIECE_BLOCK = 7,
		CANCEL = 8
	};

private:
	friend class Torrent;
	struct Block
	{
		size_t size;
		uint8_t *data;

		Block()
		{
			data = nullptr;
			size = 0;
		}
		~Block() { delete[] data; }
	};

	struct Piece
	{
		size_t index;
		size_t haveBlocks; // how much i currently have
		size_t totalBlocks; // total number of blocks i need

		~Piece() { delete[] blocks; }
		Block *blocks;
	};

	std::vector<size_t> pieces;
	std::vector<Piece *> pieceQueue;
	std::string peerId;
	uint8_t state;
	Torrent* torrent;
	Connection *conn;

	void handle(const uint8_t *data, size_t size);
	void handleMessage(MessageType mType, IncomingMessage in);
	void handleError(const std::string &);

	void sendInterested();
	void sendHave(uint32_t index);
	void sendRequest(uint32_t index, uint32_t begin, uint32_t size);
	void sendPieceRequest(uint32_t index);
	void sendCancelRequest(Piece *p);
	void sendCancel(uint32_t index, uint32_t begin, uint32_t size);

	void requestPiece(size_t pieceIndex);
	inline std::vector<size_t> getPieces() const { return pieces; }

	inline bool hasPiece(size_t index) { return std::find(pieces.begin(), pieces.end(), index) != pieces.end(); }

public:
	Peer(Torrent* t);
	~Peer();

	inline void setId(const std::string &id) { peerId = id; }
	inline std::string getStrIp() const { return conn->getIPString(); }
	inline uint32_t getRawIp() const { return conn->getIP(); }
	void disconnect();
	void connect(const std::string &ip, const std::string &port);
};

#endif
