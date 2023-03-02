#include "torrent.hpp"
#include <net/connection.hpp>
#include <utils/utils.hpp>
#include <algorithm>
#include <iostream>
#include <thread>
#include <boost/uuid/detail/sha1.hpp>
using namespace bencoding;

const int64_t MAX_BLOCK_REQUEST_SIZE = 16384;
extern std::ofstream logFile;

Torrent::Torrent() : acceptor(nullptr),
					 bitfield(0),
					 completedPieces(0),
					 uploadedBytes(0),
					 downloadedBytes(0),
					 wastedBytes(0),
					 pieceHashMisses(0),
					 pieceLength(0),
					 totalSize(0)
{
	memset(handshake, 0, 68);
	memset(peerId, 0, 20);
	activePeers.reserve(100); // use constant
}

Torrent::~Torrent()
{
	// maybe make .isOpen atomic???
	if (file.fp != nullptr)
	{
		fclose(file.fp);
		file.fp = nullptr;
	}
}

ParseResult Torrent::parseFile(const std::string &fileName, const std::string &downloadDir)
{
	std::shared_ptr<BItem> decodedData;
	std::ifstream input(fileName);
	if (!input.is_open())
	{
		std::cerr << "Failed to open torrent file: " << fileName << std::endl;
		return ParseResult::FAILED_TO_OPEN;
	}

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
	memcpy(&handshake[48], "-YT00000000000000000", 20);
	memcpy(&peerId[0], "-YT00000000000000000", 20);

	name = ((*infoDict)[BString::create("name")])->as<BString>()->value();
	pieceLength = ((*infoDict)[BString::create("piece length")])->as<BInteger>()->value();

	std::string piecesStr = ((*infoDict)[BString::create("pieces")])->as<BString>()->value();
	for (size_t i = 0; i < piecesStr.size(); i += 20)
	{
		Piece piece;
		piece.finished = false;
		piece.priority = 0;
		memcpy(&piece.hash[0], piecesStr.c_str() + i, 20);
		this->pieces.push_back(piece);
	}
	// this->bitfield.resize(this->pieces.size()); // Use this
	this->bitfield = boost::dynamic_bitset<uint8_t>(this->pieces.size());
	// std::clog << "\n\n\n" << this->bitfield.capacity() << "\n\n\n" << std::endl;

	mkdir(downloadDir.c_str(), 0700); // TODO: ADD ERROR HANDLING
	chdir(downloadDir.c_str());
	this->downloadDir = downloadDir;

	std::shared_ptr<BItem> &files = (*infoDict)[BString::create("files")];
	if (files != nullptr /*i.e. there is more than 1 file*/)
	{ // TODO: IMPLEMENT
		std::cerr << "Torrent includes multiple files in it. That functionality is still in development." << std::endl;
		return ParseResult::CORRUPTED_FILE;
	}
	else
	{
		int64_t length = ((*infoDict)[BString::create("length")])->as<BInteger>()->value();

		File file;

		if (!nodeExists(name.c_str())) // rewrite this using only this case
		{
			file.fp = fopen(name.c_str(), "wb+"); // platform independent
			if (!file.fp)
			{
				std::cerr << name << ": unable to create " << name << std::endl;
				chdir("..");
				return ParseResult::FAILED_TO_OPEN;
			}
		}
		else
		{
			file.fp = fopen(name.c_str(), "rb+"); // platform independent
			if (!file.fp)
			{
				std::cerr << name << ": unable to open " << name << std::endl;
				chdir("..");
				return ParseResult::FAILED_TO_OPEN;
			}

			std::clog << name << ": Completed pieces: " << completedPieces << "/" << pieces.size() << std::endl;
		}
		totalSize = length;
		this->file = file;
	}

	chdir("..");
	return ParseResult::SUCCESS;
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
		if (pieces[i].finished)
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
	startDownloadTime = std::chrono::high_resolution_clock::now();
	if (!validateTracker(mainTrackerUrl, buildTrackerQuery(TrackerEvent::STARTED), port))
	{
		return DownloadError::TRACKER_QUERY_FAILURE;
	}
	if (this->acceptor == nullptr)
	{
		this->acceptor = new Acceptor(port); // this could throw an exception. how do i handle it?
	}

	// async accept incoming connections
	acceptor->initiateAsyncAcceptLoop(
		[this](const std::shared_ptr<Connection> &conn)
		{
			auto peer = std::make_shared<Peer>(this, conn);
			peer->authenticate();
		});

	while (completedPieces != piecesNeeded)
	{
		if (mainTracker->isNextRequestDue())
		{
			mainTracker->query(buildTrackerQuery(TrackerEvent::NONE));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		displayDownloadProgress();
	}
	std::cout << std::endl; // make up for the last \r

	// https://stackoverflow.com/a/17941712/18301773
	if (file.fp != nullptr)
	{
		fclose(file.fp);
		file.fp = nullptr;
	}

	TrackerEvent event;
	event = (completedPieces == piecesNeeded) ? TrackerEvent::COMPLETED : TrackerEvent::STOPPED;

	mainTracker->query(buildTrackerQuery(event));
	// disconnectPeers();
	return event == TrackerEvent::COMPLETED ? DownloadError::COMPLETED : DownloadError::NETWORK_ERROR;
}

void Torrent::seed(uint16_t port)
{
	if (!validateTracker(mainTrackerUrl, buildTrackerQuery(TrackerEvent::STARTED), port))
		return;

	if (!acceptor->isStarted())
	{
		// incoming connections
		acceptor->initiateAsyncAcceptLoop(
			[this](const std::shared_ptr<Connection> &conn)
			{
				auto peer = std::make_shared<Peer>(this, conn);
				peer->authenticate();
			});
	}

	for (;;) // add some condition to stop seeding for ex. until have uploaded the file size at least once
	{
		if (mainTracker->isNextRequestDue())
		{
			mainTracker->query(buildTrackerQuery(TrackerEvent::COMPLETED));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		displaySeedProgress();
	}
	std::cout << std::endl; // make up for the last \r

	TrackerQuery q = buildTrackerQuery(TrackerEvent::STOPPED);
	mainTracker->query(q);
}

void Torrent::customDownload(std::string peerIp, std::string peerPort)
{
	startDownloadTime = std::chrono::high_resolution_clock::now();
	size_t piecesNeeded = pieces.size();
	auto peer = std::make_shared<Peer>(this);
	peer->connect(peerIp, peerPort);
	while (completedPieces != piecesNeeded)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	// https://stackoverflow.com/a/17941712/18301773
	if (file.fp != nullptr)
	{
		fclose(file.fp);
		file.fp = nullptr;
	}
}

bool Torrent::validateTracker(const std::string &turl, const TrackerQuery &q, uint16_t myPort)
{
	UrlMeta urlMeta = parseTrackerUrl(turl);
	if (urlMeta.host.empty())
	{
		std::cerr << name << ": queryTracker(): failed to parse announce url: " << turl << std::endl;
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
		for (size_t i = 0; i < activePeers.size(); i++)
		{
			if (activePeers[i]->getRawIp() == ip)
			{
				exists = true;
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
	logFile << name << ": Peers: " << activePeers.size() << std::endl;
}

void Torrent::removePeer(const std::shared_ptr<Peer> &peer, const std::string &errmsg)
{
	auto it = std::find(activePeers.begin(), activePeers.end(), peer);
	if (it != activePeers.end())
	{
		activePeers.erase(it);
	}

	logFile << name << ": " << peer->getStrIp() << ": " << errmsg << std::endl;
	logFile << name << ": Peers: " << activePeers.size() << std::endl;
}

void Torrent::disconnectPeers()
{
	// The elements of activePeers are edited asynchronously, there are race conditions
	// instead of using iterators which become invalid, manual indexing is used here
	for (size_t i = 0; i < this->activePeers.size(); i++)
	{
		activePeers[i]->disconnect();
		// activePeers[i].get()->~Peer();
		if (activePeers[i].use_count() == 4)
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

void Torrent::initiatePieceRequesting(const std::shared_ptr<Peer> &peer)
{
	size_t index = 0;
	int32_t priority = std::numeric_limits<int32_t>::max();

	for (size_t i = 0; i < pieces.size(); ++i)
	{
		Piece *p = &pieces[i];
		if (p->finished || !peer->hasPiece(i))
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

std::vector<uint8_t> Torrent::getRawBitfield() const // https://stackoverflow.com/a/9081167/18301773
{
	std::vector<uint8_t> rBitfield;
	boost::to_block_range(bitfield, std::back_inserter(rBitfield)); // this shit puts bits in lsbf and not msbf
	for (size_t i = 0; i < rBitfield.size(); i++)					// reverse bits
	{
		// this shit so ugly i wanna kms
		uint8_t b = rBitfield[i];
		b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
		b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
		b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
		rBitfield[i] = b;
	}
	return rBitfield;
}
// std::vector<uint8_t> bf;
// for(size_t i=0;i<bitfield.size();i++)
// {
// 	uint8_t b = 0;
// 	for(size_t j=0;j<8;j++)
// 	{
// 		b |= bitfield[i*8+j] << j;
// 	}
// 	bf.push_back(b);
// }

size_t Torrent::calculateETA() const
{
	// calculate remaining size to download
	size_t remaining_size = totalSize - downloadedBytes;

	// calculate ETA in seconds
	double eta_sec = static_cast<double>(remaining_size) / (getDownloadSpeed() * 1000000.0);

	return static_cast<size_t>(eta_sec + 0.5);
}

double Torrent::getDownloadSpeed() const
{
	auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - startDownloadTime).count();
	if (elapsedSec == 0)
		return 0.0;

	double speed = (double)(downloadedBytes / elapsedSec);
	return speed / 1000000;
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

void Torrent::handleTrackerError(const std::shared_ptr<Tracker> &tracker, const std::string &error)
{
	logFile << name << ": tracker request failed: " << error << std::endl;
}

void Torrent::handlePeerDebug(const std::shared_ptr<Peer> &peer, const std::string &msg)
{
	logFile << name << ": " << peer->getStrIp() << " " << msg << std::endl;
}

void Torrent::displayDownloadProgress() const
{
	std::clog << "\r" << name << ": "
			  << " Completed " << completedPieces << "/" << pieces.size() << " pieces "
			  << "(Downloaded: " << bytesToHumanReadable(downloadedBytes, true) << ", Uploaded: " << bytesToHumanReadable(uploadedBytes, true) << ", Wasted: " << bytesToHumanReadable(wastedBytes, true) << ", Piece hash misses: " << pieceHashMisses << ")"
			  << " Download speed: " << std::fixed << std::showpoint << std::setprecision(2) << getDownloadSpeed() << " MB/s"
			  << ", ETA: " << formatTime(calculateETA()) << "       "
			  << std::flush;
}

void Torrent::displaySeedProgress() const
{
	std::clog << "\r" << name << ": "
			  << "(Uploaded: " << bytesToHumanReadable(uploadedBytes, true) << ", Wasted: " << bytesToHumanReadable(wastedBytes, true) << ", Piece hash misses: " << pieceHashMisses << ")"
			  << "       "
			  << std::flush;
}

void Torrent::handlePieceCompleted(const std::shared_ptr<Peer> &peer, uint32_t index, const std::vector<uint8_t> &pieceData)
{
	if (!checkPieceHash(&pieceData[0], pieceData.size(), index))
	{
		logFile << name << ": " << peer->getStrIp() << " checksum mismatch for piece " << index << "." << std::endl;
		++pieceHashMisses;
		wastedBytes += pieceData.size();
		return;
	}

	pieces[index].finished = true;
	downloadedBytes += pieceData.size();
	this->completedPieces += 1;
	const uint8_t *data = &pieceData[0];

	size_t size = pieceData.size();
	int64_t beginPos = index * pieceLength;

	if (fseek(file.fp, beginPos - file.begin, SEEK_SET) != 0)
	{
		throw new std::runtime_error("fseek failed, errno = " + std::to_string(errno)); // errno is global
	}

	size_t wrote = fwrite(data, 1, size, file.fp); // faster than fstream because it doesn't do any buffering
	if (wrote != size)
	{
		// use file.path instead of this->name
		throw new std::runtime_error("fwrite failed in file: " + this->name + "saved to directory: " + this->downloadDir + " errno = " + std::to_string(errno));
	}

	this->bitfield.set(index);
	for (const std::shared_ptr<Peer> &peer : activePeers)
	{
		peer->sendHave(index);
	}
}


void Torrent::handleBlockRequest(const std::shared_ptr<Peer> &peer, uint32_t pIndex, uint32_t begin, uint32_t length)
{
	// Peer requested piece block from us
	if (pIndex >= pieces.size())
		return peer->disconnect();

	size_t blockEnd = begin + length;
	if (blockEnd > pieceSize(pIndex))
		return peer->disconnect();

	uint8_t *block = new uint8_t[length];
	size_t blockBegin = begin + (pIndex * pieceLength);
	size_t fileEnd = file.begin;

	if (file.fp == nullptr)
	{
		chdir(downloadDir.c_str());
		file.fp = fopen(name.c_str(), "rb"); // platform independent 
		if (!file.fp)
		{
			std::cerr << name << ": unable to open " << name << std::endl;
			return;
		}
		chdir("..");
	}

	size_t writePos = 0;
	// seek to where it begins
	fseek(file.fp, blockBegin - file.begin, SEEK_SET);
	int read = fread(&block[writePos], 1, length, file.fp); // platform independent
	if (read <= 0)
	{
		std::cerr << name << ": handleRequestBlock(): unable to read from: " << file.path.c_str() << std::endl;
		return;
	}

	peer->sendPieceBlock(pIndex, begin, block, length);
	uploadedBytes += length;
	delete[] block;
}
