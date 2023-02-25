#ifndef TRACKER_H
#define TRACKER_H

#include <string>
#include <chrono>
#include <memory>
#include <utils/utils.hpp>

enum class TrackerEvent
{
	NONE = 0,
	COMPLETED = 1,
	STARTED = 2,
	STOPPED = 3,
};

struct TrackerQuery
{
	TrackerEvent event;
	size_t downloaded;
	size_t uploaded;
	size_t remaining;
};

class Torrent;
class Tracker : public std::enable_shared_from_this<Tracker>
{
private:
	Torrent *torrent;
	uint16_t myPort;
	UrlMeta urlMeta;
	std::chrono::time_point<std::chrono::system_clock> timeToNextRequest;
	bool httpRequest(const TrackerQuery &query);

	friend class Torrent;

public:
	Tracker(Torrent *torrent, uint16_t myPort, UrlMeta urlMeta);

	// Start querying this tracker for peers etc.
	bool query(const TrackerQuery &request);
	bool isNextRequestDue() { return std::chrono::system_clock::now() >= timeToNextRequest; }
	void setNextRequestTime(const std::chrono::time_point<std::chrono::system_clock> &p) { timeToNextRequest = p; }
};
#endif
