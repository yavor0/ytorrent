#include "torrent.hpp"
#include <net/connection.hpp>
#include <utils/utils.hpp>
#include <algorithm>
#include <iostream>
#include <thread>
#include <boost/uuid/detail/sha1.hpp>
using namespace bencoding;

Torrent::Torrent() : completedPieces(0),
					 uploadedBytes(0),
					 downloadedBytes(0),
					 wastedBytes(0),
					 hashMisses(0),
					 pieceLength(0),
					 totalSize(0)
{
	activePeers.reserve(100); // use constant
}

Torrent::~Torrent()
{
	for (auto &f : files)
	{
		if (f.fp != nullptr)
		{
			fclose(f.fp);
			f.fp=nullptr;
		}
	}

	// for(size_t i =0; i<this->activePeers.size();i++)
	// {
	// 	activePeers[i]->disconnect();
	// 	// activePeers[i].get()->~Peer();
	// 	if(activePeers[i].use_count() == 4)
	// 	{
	// 		// delete activePeers[i].get();
	// 		activePeers[i].get()->~Peer();
	// 	}	
	// }
}

bool Torrent::parseFile(const std::string &fileName, const std::string &downloadDir)
{
	std::shared_ptr<BItem> decodedData;
	std::ifstream input(fileName);
	decodedData = bencoding::decode(input);
	std::shared_ptr<BDictionary> torrMetaDict = decodedData->as<BDictionary>();

	std::shared_ptr<BItem> &annVal = (*torrMetaDict)[BString::create("announce")];
	std::shared_ptr<BString> bStrAnn = annVal->as<BString>();
	mainTrackerUrl = bStrAnn->value();

	std::shared_ptr<BItem> &infoVal = (*torrMetaDict)[BString::create("info")];
	std::shared_ptr<BDictionary> infoDict = infoVal->as<BDictionary>();

	uint8_t checkSum[20];
	std::string strInfoHash = bencoding::encode(infoDict);
	boost::uuids::detail::sha1 sha1;
	sha1.process_bytes(strInfoHash.data(), strInfoHash.size());

	unsigned int sum[5];
	sha1.get_digest(sum);
	for (int i = 0; i < 5; ++i)
	{
		writeAsBE32(&checkSum[i * 4], sum[i]);
	}

	handshake[0] = 0x13; // 19 length of string "BitTorrent protocol"
	memcpy(&handshake[1], "BitTorrent protocol", 19);
	memset(&handshake[20], 0x00, 8);	  // reserved bytes	(last |= 0x01 for DHT or last |= 0x04 for FPE)
	memcpy(&handshake[28], checkSum, 20); // info hash
	memcpy(&handshake[48], "-YT00000000000000000", 8);
	memcpy(&peerId[0], "-YT00000000000000000", 20);

	name = ((*infoDict)[BString::create("name")])->as<BString>()->value();
	pieceLength = ((*infoDict)[BString::create("piece length")])->as<BInteger>()->value();

	std::string piecesStr = ((*infoDict)[BString::create("pieces")])->as<BString>()->value();
	for (size_t i = 0; i < piecesStr.size(); i += 20)
	{
		Piece piece;
		piece.done = false;
		piece.priority = 0;
		memcpy(&piece.hash[0], piecesStr.c_str() + i, 20);
		this->pieces.push_back(piece);
	}

	mkdir(downloadDir.c_str(), 0700); // TODO: ADD ERROR HANDLING
	chdir(downloadDir.c_str());

	std::string installDir = getcwd();
	std::shared_ptr<BItem> &files = (*infoDict)[BString::create("files")];
	if (files != nullptr /*i.e. there is more than 1 file*/)
	{ // TODO: IMPLEMENT
		std::cerr << "Torrent includes multiple files in it. That functionality is still in development." << std::endl;
		return false;
	}
	else
	{
		int64_t length = ((*infoDict)[BString::create("length")])->as<BInteger>()->value();

		File file = {
			.path = name.c_str(),
			.fp = nullptr,
			.begin = 0,
			.length = length};

		if (!nodeExists(name.c_str())) // BRUH
		{
			file.fp = fopen(name.c_str(), "wb");
			if (!file.fp)
			{
				std::cerr << name << ": unable to create " << name << std::endl;
				return false;
			}
		}
		else
		{
			file.fp = fopen(name.c_str(), "rb+");
			if (!file.fp)
			{
				std::cerr << name << ": unable to open " << name << std::endl;
				return false;
			}

			std::clog << name << ": Completed pieces: " << completedPieces << "/" << pieces.size() << std::endl;
		}

		totalSize = length;
		this->files.push_back(file);
	}

	chdir("..");
	return true;
}

bool Torrent::checkPieceHash(const uint8_t *data, size_t size, uint32_t index)
{
	if (index >= pieces.size())
		return false;

	uint8_t pieceHash[20];
	boost::uuids::detail::sha1 sha1;
	sha1.process_bytes(data, size);

	unsigned int sum[5];
	sha1.get_digest(sum);
	for (int i = 0; i < 5; ++i)
	{
		writeAsBE32(&pieceHash[i * 4], sum[i]);
	}

	return memcmp(pieceHash, pieces[index].hash, 20) == 0;
}

TrackerQuery Torrent::buildTrackerQuery(TrackerEvent event) const // MOVE THIS FUNCTION TO TRACKER CLASS
{
	TrackerQuery q;

	q.event = event;
	q.uploaded = 0;
	q.downloaded = 0;

	for (size_t i = 0; i < pieces.size(); ++i)
	{
		if (pieces[i].done)
		{
			q.downloaded += pieceSize(i);
		}
	}
	q.remaining = totalSize - q.downloaded;
	return q;
}

Torrent::DownloadError Torrent::download(uint16_t port)
{
	size_t piecesNeeded = pieces.size();

	if (!validateTracker(mainTrackerUrl, buildTrackerQuery(TrackerEvent::STARTED), port))
	{
		return DownloadError::TRACKER_QUERY_FAILURE;
	}

	while (completedPieces != piecesNeeded)
	{
		if (mainTracker->isNextRequestDue())
		{
			mainTracker->query(buildTrackerQuery(TrackerEvent::NONE));
			// std::cout << "\n---------------Queried again--------------" << std::endl;
			// std::cout << "Current active peers:" << activePeers.size() << std::endl
			// 		  << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	for (auto &f : files)
	{
	 	// https://stackoverflow.com/a/17941712/18301773
		if (f.fp != nullptr)
		{
			fclose(f.fp);
			f.fp=nullptr;
		}
	}

	TrackerEvent event;
	event = (completedPieces == piecesNeeded) ? TrackerEvent::COMPLETED : TrackerEvent::STOPPED;
	
	mainTracker->query(buildTrackerQuery(event));
	// disconnectPeers();
	return event == TrackerEvent::COMPLETED ? DownloadError::COMPLETED : DownloadError::NETWORK_ERROR;
}

bool Torrent::validateTracker(const std::string &furl, const TrackerQuery &q, uint16_t myPort)
{
	UrlMeta urlMeta = parseTrackerUrl(furl);
	if (urlMeta.host.empty())
	{
		std::cerr << name << ": queryTracker(): failed to parse announce url: " << furl << std::endl;
		return false;
	}

	std::shared_ptr<Tracker> tracker = std::make_shared<Tracker>(this, myPort, urlMeta); // use make_shared<>
	if (!tracker->query(q))
	{
		return false;
	}

	mainTracker = tracker;
	return true;
}

void Torrent::connectToPeers(const uint8_t *peers, size_t size)
{
	// 6 bytes each (first 4 is ip address, last 2 port) all in big endian notation
	for (size_t i = 0; i < size; i += 6)
	{
		const uint8_t *iport = peers + i;
		uint32_t ip = readAsLE32(iport);
		
		// race condition?
		bool exists = false;
		for(size_t i=0;i<activePeers.size();i++)
		{
			if(activePeers[i]->getRawIp() == ip)
			{
				exists=true;
				break;
			}
		}
		if (exists)
		{
			continue;
		}

		// Asynchronously connect to that peer, and do not add it to our
		// active peers list unless a connection was established successfully.
		auto peer = std::make_shared<Peer>(this);
		peer->connect(parseIp(ip), std::to_string(readAsBE16(iport + 4)));
	}
}

void Torrent::addPeer(const std::shared_ptr<Peer> &peer)
{
	
	activePeers.push_back(peer);
	std::clog << name << ": Peers: " << activePeers.size() << std::endl;
}

void Torrent::removePeer(const std::shared_ptr<Peer> &peer, const std::string &errmsg)
{
	auto it = std::find(activePeers.begin(), activePeers.end(), peer);
	if (it != activePeers.end())
	{
		activePeers.erase(it);
	}

	std::clog << name << ": " << peer->getStrIp() << ": " << errmsg << std::endl;
	std::clog << name << ": Peers: " << activePeers.size() << std::endl;
}

void Torrent::disconnectPeers()
{
	// The elements of activePeers are edited asynchronously, there are race conditions
	// instead of using iterators which become invalid, manual indexing is used here
	for(size_t i =0; i<this->activePeers.size();i++)
	{
		activePeers[i]->disconnect();
		// activePeers[i].get()->~Peer();
		if(activePeers[i].use_count() == 4)
		{
			delete activePeers[i].get();

		}	
	}

	// for (const std::shared_ptr<Peer> &peer : activePeers)
	// {
	// 	peer->disconnect();
	// }

	// auto begin = activePeers.begin();
	// auto end = activePeers.end();
	// for(;begin!=end;++begin)
	// {
	// 	const auto &peer = *begin;
	// 	peer->disconnect();
	// }
}

void Torrent::requestPiece(const std::shared_ptr<Peer> &peer)
{
	size_t index = 0;
	int32_t priority = std::numeric_limits<int32_t>::max();

	for (size_t i = 0; i < pieces.size(); ++i)
	{
		Piece *p = &pieces[i];
		if (p->done || !peer->hasPiece(i))
			continue;

		if (!p->priority)
		{
			p->priority = 1;
			return peer->sendPieceRequest(i);
		}

		if (priority > p->priority)
		{
			priority = p->priority;
			index = i;
		}
	}

	if (priority != std::numeric_limits<int32_t>::max())
	{
		++pieces[index].priority;
		peer->sendPieceRequest(index);
	}
}

void Torrent::handleTrackerError(const std::shared_ptr<Tracker> &tracker, const std::string &error)
{
	std::cerr << name << ": tracker request failed: " << error << std::endl;
}

void Torrent::handlePeerDebug(const std::shared_ptr<Peer> &peer, const std::string &msg)
{
	std::clog << name << ": " << peer->getStrIp() << " " << msg << std::endl;
}

void Torrent::handlePieceCompleted(const std::shared_ptr<Peer> &peer, uint32_t index, const std::vector<uint8_t> &pieceData)
{
	if (!checkPieceHash(&pieceData[0], pieceData.size(), index))
	{
		std::cerr << name << ": " << peer->getStrIp() << " checksum mismatch for piece " << index << "." << std::endl;
		++hashMisses;
		wastedBytes += pieceData.size();
		return;
	}

	pieces[index].done = true;
	downloadedBytes += pieceData.size();
	++completedPieces;

	int64_t beginPos = index * pieceLength;
	const uint8_t *data = &pieceData[0];
	size_t off = 0;
	size_t size = pieceData.size();
	for (const File &file : files)
	{
		int64_t fileEnd = file.begin + file.length;
		if (beginPos < file.begin || beginPos >= fileEnd)
			break;

		int64_t amount = fileEnd - beginPos;
		if (amount > size)
		{
			amount = size;
		}

		fseek(file.fp, beginPos - file.begin, SEEK_SET);
		size_t wrote = fwrite(data + off, 1, amount, file.fp);
		off += wrote;
		size -= wrote;
		beginPos += wrote;
	}

	for (const std::shared_ptr<Peer> &peer : activePeers)
	{
		peer->sendHave(index);
	}

	std::clog << name << ": " << peer->getStrIp() << " Completed " << completedPieces << "/" << pieces.size() << " pieces "
			  << "(Downloaded: " << bytesToHumanReadable(downloadedBytes, true) << ", Wasted: " << bytesToHumanReadable(wastedBytes, true) << ", "
			  << "Hash miss: " << hashMisses << ")"
			  << std::endl;
}

int64_t Torrent::pieceSize(size_t pieceIndex) const
{
	// Last piece can be different in size
	if (pieceIndex == pieces.size() - 1)
	{
		int64_t r = totalSize % pieceLength;
		if (r != 0)
			return r;
	}

	return pieceLength;
}
