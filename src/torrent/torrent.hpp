#ifndef TORRENT_H
#define TORRENT_H

#include "peer.hpp"
#include "tracker.hpp"
#include "bencoding/bencoding.h"

#include <vector>
#include <map>
#include <string>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iosfwd>

class Torrent
{
private:
	struct File {
		std::string path;
		FILE *fp;
		int64_t begin;		
		int64_t length;
	};

public:
	enum class DownloadError {
		Completed		 = 0,
		TrackerQueryFailure	 = 1,
		AlreadyDownloaded	 = 2,
		NetworkError		 = 3
	};

	Torrent();
	~Torrent();

	bool open(const std::string& fileName, const std::string &downloadDir);
	DownloadError download(uint16_t port);


protected:
	void rawConnectPeers(const uint8_t *peers, size_t size);

private:
	struct Piece {
		bool done;
		int32_t priority;
		uint8_t hash[20];
	};	

	std::vector<Piece> m_pieces;
	std::vector<File> m_files;

	size_t m_pieceLength;
	size_t m_totalSize;

	std::string m_name;
	std::string m_mainTracker;

	friend class Tracker;
};

#endif

