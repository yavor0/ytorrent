#include "tracker.h"
#include "torrent.h"
#include <random>
#include <boost/array.hpp>

bool Tracker::query(const TrackerQuery &req) // future abstraction for whenever udp tracker querying functionality is implemented
{
	return httpRequest(req);
}

bool Tracker::httpRequest(const TrackerQuery &r)
{
	asio::ip::tcp::resolver resolver(g_service);
	asio::ip::tcp::resolver::query query(m_host, m_port);
	boost::system::error_code error;

	asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(query, error);
	asio::ip::tcp::resolver::iterator end;
	if (error) {
		m_torrent->handleTrackerError(shared_from_this(), "Unable to resolve host: " + error.message());
		return false;
	}

	asio::ip::tcp::socket socket(g_service);
	do {
		socket.close();
		socket.connect(*endpoint++, error);
	} while (error && endpoint != end);
	if (error) {
		m_torrent->handleTrackerError(shared_from_this(), "Failed to connect: " + error.message());
		return false;
	}

	std::string req;
	switch (r.event) {
	case TrackerEvent::None:	req = ""; break;			// prevent a useless warning
	case TrackerEvent::Completed: 	req = "event=completed&"; break;
	case TrackerEvent::Stopped:	req = "event=stopped&"; break;
	case TrackerEvent::Started:	req = "event=started&"; break;
	}

	// Used to extract some things.
	const uint8_t *handshake = m_torrent->handshake();
	char infoHash[20];
	memcpy(infoHash, &handshake[28], 20);

	asio::streambuf params;
	std::ostream buf(&params);


    buf << "GET " << "/announce?"
        << "info_hash=" << urlEncode(std::string(infoHash, 20))
		<< "&peer_id=" << urlEncode(std::string((const char *)m_torrent->peerId(), 20)) // huh??? remove necessity for urlencode
        << "&port=" << m_tport
        << "&uploaded=" << r.uploaded
		<< "&downloaded=" << r.downloaded
        << "&left=" << r.remaining
		<< "&compact=1"
        << " HTTP/1.0\r\n"
        << "User-Agent: YtoRRenT"
        << "\r\n"
        << "Connection: close"
        << "\r\n"
        << "\r\n";

	asio::write(socket, params);
	asio::streambuf response;
	try {
		asio::read_until(socket, response, "\r\n");
	} catch (const std::exception &e) {
		m_torrent->handleTrackerError(shared_from_this(), "Unable to read data: " + std::string(e.what()));
		return false;
	}

	std::istream rbuf(&response);
	std::string httpVersion;
	rbuf >> httpVersion;

	if (httpVersion.substr(0, 5) != "HTTP/") {
		m_torrent->handleTrackerError(shared_from_this(), "Tracker send an invalid HTTP version response");
		return false;
	}

	int status;
	rbuf >> status;
	if (status != 200) {
		std::ostringstream os;
		os << "Tracker failed to process our request: " << status;
		m_torrent->handleTrackerError(shared_from_this(), os.str());
		return false;
	}

	try {
		asio::read_until(socket, response, "\r\n\r\n");
	} catch (const std::exception &e) {
		m_torrent->handleTrackerError(shared_from_this(), "Unable to read data: " + std::string(e.what()));
		return false;
	}
	socket.close();

	// Seek to start of body
	std::string header;
	while (std::getline(rbuf, header) && header != "\r");
	if (!rbuf) {
		m_torrent->handleTrackerError(shared_from_this(), "Unable to get to tracker response body.");
		return false;
	}

	std::ostringstream os;
	os << &response;

    std::string trackerResp = os.str();
    std::shared_ptr<bencoding::BItem> bRespDict = bencoding::decode(trackerResp);
    std::shared_ptr<bencoding::BDictionary> respDict = bRespDict->as<bencoding::BDictionary>();
    std::string peersInfo = ((*respDict)[bencoding::BString::create("peers")])->as<bencoding::BString>()->value(); // ADD ERROR HANDLING
	int64_t trackerTimeout = ((*respDict)[bencoding::BString::create("interval")])->as<bencoding::BInteger>()->value();

	// TODO: Implement error handling for when the tracker returns an empty dictionary and a failure reason flag 

	m_timeToNextRequest = std::chrono::system_clock::now()
				+ std::chrono::milliseconds(trackerTimeout);
	m_torrent->rawConnectPeers((const uint8_t *)peersInfo.c_str(), peersInfo.length());;

	return true;
}

