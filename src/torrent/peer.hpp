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
	enum State : uint8_t {
		AmChoked = 1 << 0,			// I choked this peer (i.e. we're not giving him anymore pieces)
		AmInterested = 1 << 1,		// I'm interested in this peer's pieces
		PeerChoked = 1 << 2,			// Peer choked me
		PeerInterested = 1 << 3,		// Peer interested in my stuff
	};

	enum MessageType : uint8_t {
		CHOKE		= 0,
		UNCHOKE		= 1,
		INTERESTED		= 2,
		NOT_INTERESTED	= 3,
		HAVE			= 4,
		BITFIELD		= 5,
		REQUEST		= 6,
		PIECE_BLOCK		= 7,
		CANCEL		= 8,
		PORT			= 9
	};

private:
	friend class Torrent;
	struct Block {
		size_t size;
		uint8_t *data;

		Block() { data = nullptr; size = 0; }
		~Block() { delete []data; }
	};

	struct Piece {
		size_t index;
		size_t currentBlocks;
		size_t numBlocks;

		~Piece() { delete []blocks; }
		Block *blocks;
	};

	std::vector<size_t> pieces;
	std::vector<Piece *> pieceQueue;
	std::string peerId;
	uint8_t state;
	Torrent *torrent;
	Connection *conn;

	void handle(const uint8_t *data, size_t size);
	void handleMessage(MessageType mType, IncomingMessage in);
	void handleError(const std::string &);


public:
	Peer(Torrent *t);
	~Peer();

	inline void setId(const std::string &id) { peerId = id; }
	inline std::string getStrIp() const { return conn->getIPString(); }
	inline uint32_t getRawIp() const { return conn->getIP(); }
	void disconnect();
	void connect(const std::string &ip, const std::string &port);
};


#endif

