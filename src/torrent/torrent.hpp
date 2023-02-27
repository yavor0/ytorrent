#ifndef TORRENT_H
#define TORRENT_H

#include "peer.hpp"
#include "tracker.hpp"
#include "bencoding/bencoding.h"
#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>

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
	File file;

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

	std::chrono::high_resolution_clock::time_point startDownloadTime;
	uint8_t handshake[68];
	uint8_t peerId[20];

	friend class Peer;
	friend class Tracker;

	bool checkPieceHash(const uint8_t *data, size_t size, uint32_t index);
	bool validateTracker(const std::string &url, const TrackerQuery &r, uint16_t myPort);
	void connectToPeers(const uint8_t *peers, size_t size);
	void requestPiece(const std::shared_ptr<Peer> &peer, size_t pieceIndex);
	void requestPiece(const std::shared_ptr<Peer> &peer);
	int64_t pieceSize(size_t pieceIndex) const;
	inline bool pieceDone(size_t pieceIndex) const { return pieces[pieceIndex].done; }

	void addPeer(const std::shared_ptr<Peer> &peer);
	void removePeer(const std::shared_ptr<Peer> &peer, const std::string &errmsg);
	void disconnectPeers();
	inline const uint8_t *getPeerId() const { return peerId; }
	inline const uint8_t *getHandshake() const { return handshake; }
	inline size_t getTotalPieces() const { return pieces.size(); }
	inline size_t getCompletedPieces() const { return completedPieces; }
	size_t calculateETA() const;
	double getDownloadSpeed() const;

	TrackerQuery buildTrackerQuery(TrackerEvent event) const;
	void handleTrackerError(const std::shared_ptr<Tracker> &tracker, const std::string &error);
	void handlePeerDebug(const std::shared_ptr<Peer> &peer, const std::string &msg);
	void handlePieceCompleted(const std::shared_ptr<Peer> &peer, uint32_t index, const std::vector<uint8_t> &data);
	void handleRequestBlock(const std::shared_ptr<Peer> &peer, uint32_t index, uint32_t begin, uint32_t length);
public:
	// std::mutex removeLock;
	enum class DownloadError
	{
		COMPLETED,
		TRACKER_QUERY_FAILURE,
		NETWORK_ERROR
	};

	Torrent();
	~Torrent();

	bool parseFile(const std::string &fileName, const std::string &downloadDir);
	DownloadError download(uint16_t port);

	inline bool isFullyDownloaded() const { return completedPieces == pieces.size(); }
	inline size_t getActivePeers() const { return activePeers.size(); }
	inline int64_t getTotalSize() const { return totalSize; }
	inline int64_t getDownloadedBytes() const { return downloadedBytes; }
	inline uint32_t getUploadedBytes() const { return uploadedBytes; }
	inline std::string getName() const { return name; }
};

#endif
