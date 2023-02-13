#include "peer.hpp"
#include "torrent.hpp"
#include <vector>
#include <iostream>

Peer::Peer(Torrent *torrent)
	: m_torrent(torrent)
{
	m_conn = new Connection();
	m_state = PS_AmChoked | PS_PeerChoked;
}

Peer::~Peer()
{
	delete m_conn;
	for (Piece *p : m_queue)
		delete p;
	m_queue.clear();
}
void Peer::connect(const std::string &ip, const std::string &port)
{
	m_conn->setErrorCallback(std::bind(&Peer::handleError, shared_from_this(), std::placeholders::_1));
	m_conn->connect(ip, port,
		[this] ()
		{
			const uint8_t *m_handshake = m_torrent->handshake();
			m_conn->write(m_handshake, 68);
			m_conn->read_partial(68,
				[this, m_handshake] (const uint8_t *handshake, size_t size)
				{
					if (size != 68
						|| (handshake[0] != 0x13 && memcmp(&handshake[1], "BitTorrent protocol", 19) != 0)
						|| memcmp(&handshake[28], &m_handshake[28], 20) != 0)
						return handleError("info hash/protocol type mismatch");

					std::string peerId((const char *)&handshake[48], 20);
					if (!m_peerId.empty() && peerId != m_peerId)
						return handleError("unverified");

					m_peerId = peerId;
					m_torrent->addPeer(shared_from_this());
					m_conn->read_partial(4, std::bind(&Peer::handle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
				}
			);
		}
	);
}


void Peer::handle(const uint8_t *data, size_t size)
{
	if (size != 4)
		return handleError("Peer::handle(): Expected 4-byte length");

	uint32_t length = readAsBE32(data);
	switch (length) {
	case 0: // Keep alive
		return m_conn->read(4, std::bind(&Peer::handle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	default:
		m_conn->read(length,
			[this] (const uint8_t *data, size_t size)
			{
				InputMessage in(const_cast<uint8_t *>(&data[1]), size - 1);
				handleMessage((MessageType)data[0], in);
			}
		);
	}
}
void Peer::handleMessage(MessageType messageType, InputMessage in)
{
	size_t payloadSize = in.getSize();

	switch (messageType) {
	case CHOKE:
		if (payloadSize != 0)
			return handleError("invalid choke-message size");

		torrent->handlePeerDebug(shared_from_this(), "choke");
		state |= PeerChoked;
		break;
	case UNCHOKE:
		if (payloadSize != 0)
			return handleError("invalid unchoke-message size");

		state &= ~PeerChoked;
		torrent->handlePeerDebug(shared_from_this(), "unchoke");

		for (Piece *piece : pieceQueue)
		{
			requestPiece(piece->index);
		}

		break;
	case UNCHOKE:
		if (payloadSize != 0)
			return handleError("invalid unchoke-message size");

		state &= ~PeerChoked;
		torrent->handlePeerDebug(shared_from_this(), "unchoke");

		for (Piece *piece : pieceQueue)
		{
			requestPiece(piece->index);
		}

		break;
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
	case BITFIELD:
	case REQUEST:
	case PIECE_BLOCK:
	case CANCEL:

	}

}
void Peer::requestPiece(size_t pieceIndex)
{
	if (test_bit(state, PeerChoked))
		return handleError("Attempt to request piece from a peer that is remotely choked");

	size_t begin = 0;
	size_t length = torrent->pieceSize(pieceIndex);
	for (; length > maxRequestSize; length -= maxRequestSize, begin += maxRequestSize)
	{
		sendRequest(pieceIndex, begin, maxRequestSize);
	}

	sendRequest(pieceIndex, begin, length);
}



