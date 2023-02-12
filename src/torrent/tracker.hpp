#ifndef TRACKER_H
#define TRACKER_H

#include <string>
#include <chrono>
#include <memory>

enum class TrackerEvent {
	NONE = 0,
	COMPLETED = 1,
	STARTED = 2,
	STOPPED = 3,
};

struct TrackerQuery {
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
std::chrono::time_point<std::chrono::system_clock> timeToNextRequest;

uint16_t rawPort;
std::string host;
std::string strPort;
std::string protocol;
bool httpRequest(const TrackerQuery &query);

friend class Torrent;

public:
	Tracker(Torrent *torrent, const std::string &host, const std::string &strPort, const std::string &protocol, uint16_t rawPort);

	// Start querying this tracker for peers etc.
	bool query(const TrackerQuery &request);
	bool isNextRequestDue(void) { return std::chrono::system_clock::now() >= timeToNextRequest; }
	void setNextRequestTime(const std::chrono::time_point<std::chrono::system_clock> &p) { timeToNextRequest = p; }
};
#endif

