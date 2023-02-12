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
	case MT_Choke:
	case MT_UnChoke:
	case MT_Interested:
	case MT_NotInterested:
	case MT_Bitfield:
	case MT_Request:
	case MT_PieceBlock:
	case MT_Cancel:
	case MT_Port:
	}

}



