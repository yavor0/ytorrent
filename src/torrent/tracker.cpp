#include "tracker.hpp"
#include "torrent.hpp"
#include <random>

Tracker::Tracker(Torrent *torrent, uint16_t myPort, UrlMeta urlMeta)
	: torrent(torrent),
	  myPort(myPort),
	  urlMeta(urlMeta)
{
}

bool Tracker::query(const TrackerQuery &query) // future abstraction for whenever udp tracker querying functionality is implemented
{
	return httpRequest(query);
}

bool Tracker::httpRequest(const TrackerQuery &r)
{
	asio::ip::tcp::resolver resolver(g_io_context);
	asio::ip::tcp::resolver::query query(urlMeta.host, urlMeta.port);
	boost::system::error_code error;

	asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(query, error);
	asio::ip::tcp::resolver::iterator end;
	if (error)
	{
		torrent->handleTrackerError(shared_from_this(), "Unable to resolve host: " + error.message());
		return false;
	}

	asio::ip::tcp::socket socket(g_io_context);
	do
	{
		socket.close();
		socket.connect(*endpoint++, error);
	} while (error && endpoint != end);
	if (error)
	{
		torrent->handleTrackerError(shared_from_this(), "Failed to connect: " + error.message());
		return false;
	}

	std::string event;
	switch (r.event)
	{
	case TrackerEvent::NONE:
		event = "";
		break; // prevent a useless warning
	case TrackerEvent::COMPLETED:
		event = "event=completed&";
		break;
	case TrackerEvent::STOPPED:
		event = "event=stopped&";
		break;
	case TrackerEvent::STARTED:
		event = "event=started&";
		break;
	}

	// Used to extract some things.
	const uint8_t *handshake = torrent->getHandshake();
	char infoHash[20];
	memcpy(infoHash, &handshake[28], 20);

	asio::streambuf params;
	std::ostream buf(&params);

	buf << "GET "
		<< urlMeta.announcePath << "?"
		<< event
		<< "info_hash=" << urlEncode(std::string(infoHash, 20))
		<< "&peer_id=" << urlEncode(std::string((const char *)torrent->getPeerId(), 20)) // huh??? remove necessity for urlencode
		<< "&port=" << myPort
		<< "&uploaded=" << r.uploaded
		<< "&downloaded=" << r.downloaded
		<< "&left=" << r.remaining
		<< "&compact=1"
		<< urlEncode(urlMeta.passKeyParam)
		<< " HTTP/1.0\r\n"
		<< "Host: " << urlMeta.host
		<< "\r\n"
		<< "User-Agent: YtoRRenT"
		<< "\r\n"
		<< "Connection: close"
		<< "\r\n"
		<< "\r\n";

	asio::write(socket, params);
	asio::streambuf response;
	try
	{
		asio::read_until(socket, response, "\r\n");
	}
	catch (const std::exception &e)
	{
		torrent->handleTrackerError(shared_from_this(), "Unable to read data: " + std::string(e.what()));
		return false;
	}

	std::istream rbuf(&response);
	std::string httpVersion;
	rbuf >> httpVersion;

	if (httpVersion.substr(0, 5) != "HTTP/")
	{
		torrent->handleTrackerError(shared_from_this(), "Tracker send an invalid HTTP version response");
		return false;
	}

	int status;
	rbuf >> status;
	if (status != 200)
	{
		std::ostringstream os;
		os << "Tracker failed to process our request: " << status;
		torrent->handleTrackerError(shared_from_this(), os.str());
		return false;
	}

	try
	{
		asio::read_until(socket, response, "\r\n\r\n");
	}
	catch (const std::exception &e)
	{
		torrent->handleTrackerError(shared_from_this(), "Unable to read data: " + std::string(e.what()));
		return false;
	}
	socket.close();

	// Seek to start of body
	std::string header;
	while (std::getline(rbuf, header) && header != "\r");
	if (!rbuf)
	{
		torrent->handleTrackerError(shared_from_this(), "Unable to get to tracker response body.");
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

	timeToNextRequest = std::chrono::system_clock::now() + std::chrono::milliseconds(trackerTimeout);
	if (r.event == TrackerEvent::NONE || r.event == TrackerEvent::STARTED)
	{
		torrent->connectToPeers((const uint8_t *)peersInfo.c_str(), peersInfo.length());
	}

	return true;
}
