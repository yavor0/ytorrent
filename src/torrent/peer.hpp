#ifndef PEER_H
#define PEER_H

#include <net/connection.hpp>
#include <net/incomingmessage.hpp>

#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

class Torrent;
class Peer : public std::enable_shared_from_this<Peer>
{
	struct Piece;

	enum State : uint8_t
	{
		AM_CHOKING = 0,
		AM_INTERESTED = 1,
		PEER_CHOKED = 2,
		PEER_INTERESTED = 3
	};

	enum MessageID : uint8_t
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

	std::vector<size_t> hasPieceIndexes;
	std::vector<Piece *> pieceQueue;
	std::string peerId;
	Torrent* torrent;
	boost::dynamic_bitset<> state;
	std::shared_ptr<Connection> conn;

	void handle(const uint8_t *data, size_t size);
	void handleMessage(MessageID messageType, IncomingMessage in);
	void handleError(const std::string &msg);

	void sendInterested();
	void sendHave(uint32_t index);
	void sendBitfield(std::vector<uint8_t> rBitfield);
	void sendRequest(uint32_t index, uint32_t begin, uint32_t size);
	void sendPieceRequest(uint32_t index);
	void sendPieceBlock(uint32_t index, uint32_t begin, uint8_t *block, uint32_t size);
	void sendCancelRequest(Piece *p);
	void sendCancel(uint32_t index, uint32_t begin, uint32_t size);

	void requestPiece(size_t pieceIndex);
	inline std::vector<size_t> getPieces() const { return hasPieceIndexes; }

	inline bool hasPiece(size_t index) { return std::find(hasPieceIndexes.begin(), hasPieceIndexes.end(), index) != hasPieceIndexes.end(); }

	void authenticate();
public:
	Peer(Torrent* t);
	Peer(Torrent* t, std::shared_ptr<Connection>& conn);
	~Peer(); // this is noexcept

	void simulateDestructor();
	inline void setId(const std::string &id) { peerId = id; }
	inline std::string getStrIp() const { return conn->getIPString(); }
	inline uint32_t getRawIp() const { return conn->getIP(); }
	void disconnect();
	void connect(const std::string &ip, const std::string &port);
};

#endif
