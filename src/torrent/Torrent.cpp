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
    buf << "GET " << tm.announce.announcePath << "?"
        << "info_hash=" << urlEncode(strInfoHash) << "&peer_id=-YT00000000000000000" << "&port=6881"
        << "&uploaded=" << 0 << "&downloaded=" << 0
        << "&left=" << tm.lengthSum << "&compact=1"
        << tm.announce.passKeyParam
        << " HTTP/1.0\r\n"
        << "Host: " << tm.announce.host << "\r\n"
        << "User-Agent: myTestTorrent" << "\r\n"
        << "Connection: close" << "\r\n"
        << "\r\n";
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
