#ifndef PEER_H
#define PEER_H

#include <net/connection.h>
#include <net/inputmessage.h>

#include <memory>
#include <vector>

class Torrent;
class Peer : public std::enable_shared_from_this<Peer>
{
	struct Piece;
	enum State : uint8_t {
		PS_AmChoked = 1 << 0,			// We choked this peer (aka we're not giving him anymore pieces)
		PS_AmInterested = 1 << 1,		// We're interested in this peer's pieces
		PS_PeerChoked = 1 << 2,			// Peer choked us
		PS_PeerInterested = 1 << 3,		// Peer interested in our stuff
	};

	enum MessageType : uint8_t {
		MT_Choke		= 0,
		MT_UnChoke		= 1,
		MT_Interested		= 2,
		MT_NotInterested	= 3,
		MT_Have			= 4,
		MT_Bitfield		= 5,
		MT_Request		= 6,
		MT_PieceBlock		= 7,
		MT_Cancel		= 8,
		MT_Port			= 9
	};

public:
	Peer(Torrent *t);
	~Peer();

	inline void setId(const std::string &id) { m_peerId = id; }
	inline std::string getIP() const { return m_conn->getIPString(); }
	inline uint32_t ip() const { return m_conn->getIP(); }
	void disconnect();
	void connect(const std::string &ip, const std::string &port);

protected:
	void handle(const uint8_t *data, size_t size);
	void handleMessage(MessageType mType, InputMessage in);

private:
	struct PieceBlock {
		size_t size;
		uint8_t *data;

		PieceBlock() { data = nullptr; size = 0; }
		~PieceBlock() { delete []data; }
	};

	struct Piece {
		size_t index;
		size_t currentBlocks;
		size_t numBlocks;

		~Piece() { delete []blocks; }
		PieceBlock *blocks;
	};

	std::string m_peerId;
	uint8_t m_state;
	Torrent *m_torrent;
	Connection *m_conn;

	friend class Torrent;
};


#endif

