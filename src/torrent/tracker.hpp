#ifndef TRACKER_H
#define TRACKER_H

#include <string>
#include <chrono>
#include <memory>

typedef std::chrono::time_point<std::chrono::system_clock> TimePoint;

enum class TrackerEvent {
	None = 0,
	Completed = 1,
	Started = 2,
	Stopped = 3,
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
public:
	Tracker(Torrent *torrent, const std::string &host, const std::string &port, const std::string &proto, uint16_t tport)
		: m_torrent(torrent),
		  m_tport(tport),
		  m_host(host),
		  m_port(port),
		  m_prot(proto)
	{
	}

	// Start querying this tracker for peers etc.
	bool query(const TrackerQuery &request);
	bool timeUp(void) { return std::chrono::system_clock::now() >= m_timeToNextRequest; }
	void setNextRequestTime(const TimePoint &p) { m_timeToNextRequest = p; }

protected:
	bool httpRequest(const TrackerQuery &r);

private:
	Torrent *m_torrent;
	TimePoint m_timeToNextRequest;

	uint16_t m_tport;
	std::string m_host;
	std::string m_port;
	std::string m_prot;

	friend class Torrent;
};
typedef std::shared_ptr<Tracker> TrackerPtr;

#endif

