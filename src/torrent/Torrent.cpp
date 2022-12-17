#include "Torrent.hpp"

#include <iostream>

Torrent::Torrent(TorrentMeta &tm)
    : tm(tm)
{
}

void Torrent::trackerQuery()
{
    boost::asio::ip::tcp::resolver resolver(context);
    boost::asio::ip::tcp::resolver::query query(tm.announce.host, tm.announce.port);
    boost::system::error_code error;

    boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(query, error);
    boost::asio::ip::tcp::resolver::iterator end;
    if (error)
    {
        std::cout << "Unable to resolve host: " + error.message();
        return;
    }

    boost::asio::ip::tcp::socket socket(context);
    do
    {
        socket.close();
        socket.connect(*endpoint++, error);
    } while (error && endpoint != end);
    if (error)
    {
        std::cout << "Unable to resolve host: " + error.message();
        return;
    }

    boost::asio::streambuf params;
    std::ostream buf(&params);

    auto infoHashArr = getBytesEndianIndependent(tm.infoHash);
    std::string strInfoHash(std::begin(infoHashArr), std::end(infoHashArr));
    const char myPeerId[20] = "-YT0000000000000000";
    buf << "GET /announce?" << "event=started"
        << "&info_hash=" << urlEncode(strInfoHash) << "&port=6881" << "&compact=1"
        << "&peer_id=-YT00000000000000000" << "&downloaded=" << 0 << "&uploaded=" << 0
        << "&left=" << tm.lengthSum << " HTTP/1.0\r\n"
        << "Host: " << tm.announce.host << "\r\n\r\n";
    boost::asio::write(socket, params);

    boost::asio::streambuf response;
    try
    {
        boost::asio::read_until(socket, response, "\r\n");
    }
    catch (const std::exception &e)
    {
        std::cout << "Unable to read data: " + std::string(e.what());
        return;
    }

    std::istream rbuf(&response);
    std::string httpVersion;
    rbuf >> httpVersion;

    if (httpVersion.substr(0, 5) != "HTTP/")
    {
        std::cout << "Tracker send an invalid HTTP version response";
        return;
    }

    int status;
    rbuf >> status;
    if (status != 200)
    {
        std::cout << "Response code not 200: " << status << std::endl;
        // return;
    }

    try
    {
        boost::asio::read_until(socket, response, "\r\n\r\n");
    }
    catch (const std::exception &e)
    {
        std::cout << "Unable to read data: " + std::string(e.what());
        return;
    }
    socket.close();

    std::cout << &response << std::endl;
}

// boost::asio::ip::tcp::iostream s;

// // The entire sequence of I/O operations must complete within 60 seconds.
// // If an expiry occurs, the socket is automatically closed and the stream
// // becomes bad.
// s.expires_after(std::chrono::seconds(60));
// UrlMeta m = tm.get_announce();
// s.connect(m.host, m.port);
// if (!s)
// {
//     std::cout << "Unable to connect: " << s.error().message() << "\n";
//     return;
// }

// // Send the request. We specify the "Connection: close" header so that the
// // server will close the socket after transmitting the response. This will
// // allow us to treat all data up until the EOF as the content.

// auto infoHashArr = getBytesEndianIndependent(tm.infoHash);
// std::string strInfoHash(std::begin(infoHashArr), std::end(infoHashArr));
// const char myPeerId[20] = "-YT0000000000000000";
// std::cout << urlEncode(strInfoHash) << std::endl;
// std::cout << urlEncode(myPeerId) << std::endl;
// s << "GET /announce?" << "info_hash=" << urlEncode(strInfoHash) << "&compact=1"
// << "&peer_id=" << urlEncode(myPeerId) << "&downloaded=" << 0 << "&uploaded=" << 0
// << "&left=" << tm.lengthSum << " HTTP/1.0\r\n"
// << "Host: " << tm.announce.host << "\r\n\r\n";

// //  " HTTP/1.0\r\n";
// // s << "Host: " << argv[1] << "\r\n";
// // s << "Accept: */*\r\n";
// // s << "Connection: close\r\n\r\n";

// // buf << "GET /announce?" << req << "info_hash=" << urlencode(std::string(infoHash, 20)) << "&port=" << m_tport << "&compact=1&key=1337T0RRENT"
// // << "&peer_id=" << urlencode(std::string((const char *)m_torrent->peerId(), 20)) << "&downloaded=" << r.downloaded << "&uploaded=" << r.uploaded
// // << "&left=" << r.remaining << " HTTP/1.0\r\n"
// // << "Host: " << m_host << "\r\n"
// // << "\r\n";

// // Check that response is OK.
// std::string http_version;
// s >> http_version;
// unsigned int status_code;
// s >> status_code;
// std::string status_message;
// std::getline(s, status_message);
// if (!s || http_version.substr(0, 5) != "HTTP/")
// {
//     std::cout << "Invalid response\n";
//     return;
// }
// if (status_code != 200)
// {
//     std::cout << "Response returned with status code " << status_code << "\n";
//     // return ;
// }

// // Process the response headers, which are terminated by a blank line.
// std::string header;
// while (std::getline(s, header) && header != "\r")
//     std::cout << header << "\n";
// std::cout << "\n";

// // Write the remaining data to output.
// std::cout << s.rdbuf();
