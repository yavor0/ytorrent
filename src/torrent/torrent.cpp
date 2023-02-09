#include "torrent.h"

#include <net/connection.h>
#include <utils/utils.h>
#include <random>
#include <algorithm>
#include <iostream>
#include <thread>

#include <boost/uuid/detail/sha1.hpp>
using namespace bencoding;


Torrent::Torrent()
	: m_completedPieces(0),
	  m_uploadedBytes(0),
	  m_downloadedBytes(0),
	  m_wastedBytes(0),
	  m_hashMisses(0),
	  m_pieceLength(0),
	  m_totalSize(0)
{

}

Torrent::~Torrent()
{
	for (auto f : m_files)
		fclose(f.fp);
	m_files.clear();
	m_activeTrackers.clear();
}

bool Torrent::open(const std::string &fileName, const std::string &downloadDir)
{
    std::shared_ptr<BItem> decodedData;
    std::ifstream input(fileName);
    decodedData = bencoding::decode(input);
    std::shared_ptr<BDictionary> torrMetaDict = decodedData->as<BDictionary>();

    std::shared_ptr<BItem> &annVal = (*torrMetaDict)[BString::create("announce")]; //
    std::shared_ptr<BString> bStrAnn = annVal->as<BString>();
    m_mainTracker = bStrAnn->value();

	std::shared_ptr<BItem> &infoVal = (*torrMetaDict)[BString::create("info")];
    std::shared_ptr<BDictionary> infoDict = infoVal->as<BDictionary>();

	uint8_t checkSum[20];
    std::string strInfoHash = bencoding::encode(infoDict);
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(strInfoHash.data(), strInfoHash.size());

	unsigned int sum[5];
    sha1.get_digest(sum);
	for (int i = 0; i < 5; ++i)
		writeAsBE32(&checkSum[i * 4], sum[i]);
	
	
	m_handshake[0] = 0x13;					// 19 length of string "BitTorrent protocol"
	memcpy(&m_handshake[1], "BitTorrent protocol", 19);
	memset(&m_handshake[20], 0x00, 8);			// reserved bytes	(last |= 0x01 for DHT or last |= 0x04 for FPE)
	memcpy(&m_handshake[28], checkSum, 20);			// info hash
	memcpy(&m_handshake[48], "-YT00000", 8);		// Azureus-style peer id (-CT0000-XXXXXXXXXXXX)
	memcpy(&m_peerId[0], "-YT00000", 8);

	static std::random_device rd;
	static std::ranlux24 generator(rd());
	static std::uniform_int_distribution<uint8_t> random(0x00, 0xFF);
	for (size_t i = 8; i < 20; ++i)
		m_handshake[56 + i] = m_peerId[i] = random(generator);


	m_name = ((*infoDict)[BString::create("name")])->as<BString>()->value();
	m_pieceLength = ((*infoDict)[BString::create("piece length")])->as<BInteger>()->value();


	std::string pieces = ((*infoDict)[BString::create("pieces")])->as<BString>()->value();
	for (size_t i = 0; i < pieces.size(); i += 20) {
		Piece piece;
		piece.done = false;
		piece.priority = 0;
		memcpy(&piece.hash[0], pieces.c_str() + i, 20);
		m_pieces.push_back(piece);
	}

	MKDIR(downloadDir);
	chdir(downloadDir.c_str());
	
	std::string installDir = getcwd();
	std::shared_ptr<BItem> &files = (*infoDict)[BString::create("files")];
	if (files != nullptr /*i.e. there is more than 1 file*/) { // TODO: IMPLEMENT
		std::cerr << "Torrent includes multiple files in it. That functionality is still in development." << std::endl;
		return false;
	}
	else {
		int64_t length = ((*infoDict)[BString::create("length")])->as<BInteger>()->value();

		File file = {
			.path = m_name.c_str(),
			.fp = nullptr,
			.begin = 0,
			.length = length
		};
		if (!nodeExists(m_name.c_str())) {
			file.fp = fopen(m_name.c_str(), "wb");
			if (!file.fp) {
				std::cerr << m_name << ": unable to create " << m_name << std::endl;
				return false;
			}
		} else {
			file.fp = fopen(m_name.c_str(), "rb+");
			if (!file.fp) {
				std::cerr << m_name << ": unable to open " << m_name << std::endl;
				return false;
			}

			std::clog << m_name << ": Completed pieces: " << m_completedPieces << "/" << m_pieces.size() << std::endl;
		}

		m_totalSize = length;
		m_files.push_back(file);
	}

	chdir("..");
	return true;
}

void Torrent::rawConnectPeers(const uint8_t *peers, size_t size)
{
	m_peers.reserve(size / 6);

	// 6 bytes each (first 4 is ip address, last 2 port) all in big endian notation
	for (size_t i = 0; i < size; i += 6) {
		const uint8_t *iport = peers + i;
		uint32_t ip =isLittleEndian() ? readAsLE32(iport) : readAsBE32(iport);

		auto it = std::find_if(m_peers.begin(), m_peers.end(),
				[ip] (const PeerPtr &peer) { return peer->ip() == ip; });
		if (it != m_peers.end())
			continue;

		// Asynchronously connect to that peer, and do not add it to our
		// active peers list unless a connection was established successfully.
		auto peer = std::make_shared<Peer>(this);
		peer->connect(parseIp(ip), std::to_string(readAsBE16(iport + 4)));
	}
}

