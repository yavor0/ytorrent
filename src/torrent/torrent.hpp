#ifndef TORRENT_H
#define TORRENT_H

#include "peer.hpp"
#include "tracker.hpp"
#include "bencoding/bencoding.h"
#include <vector>
#include <string>
#include <fstream>

static int64_t maxRequestSize = 16384; // 16KiB initial (per piece)
class Torrent
{
private:
	struct File
	{
		std::string path;
		FILE *fp;
		int64_t begin; // offset of which the piece(s) begin
		int64_t length;
	};
	struct Piece
	{
		bool done;
		int32_t priority;
		uint8_t hash[20];
	};

	std::vector<std::shared_ptr<Peer>> activePeers; // make this thread-safe??
	std::vector<Piece> pieces;
	std::vector<File> files;

	size_t completedPieces;
	size_t uploadedBytes;
	size_t downloadedBytes;
	size_t wastedBytes;
	size_t hashMisses;

	size_t pieceLength;
	size_t totalSize;

	std::string name;
	std::string mainTrackerUrl;
	std::shared_ptr<Tracker> mainTracker;

	uint8_t handshake[68];
	uint8_t peerId[20];

	friend class Peer;
	friend class Tracker;

	void connectToPeers(const uint8_t *peers, size_t size);
	TrackerQuery buildTrackerQuery(TrackerEvent event) const;
    handlePeerDebug(const std::shared_ptr<Peer> &peer, const std::string &msg);


public:
	enum class DownloadError
	{
		COMPLETED = 0,
		TRACKER_QUERY_FAILURE = 1,
		ALREADY_DOWNLOADED = 2,
		NETWORK_ERROR = 3
	};

	Torrent();
	~Torrent();

	bool open(const std::string &fileName, const std::string &downloadDir);
	DownloadError download(uint16_t port);

	inline size_t getActivePeers() const { return activePeers.size(); }
	inline int64_t getTotalSize() const { return totalSize; }
	inline int64_t getDownloadedBytes() const { return downloadedBytes; }
	inline uint32_t getUploadedBytes() const { return uploadedBytes; }
	inline std::string getName() const { return name; }
};

#endif
