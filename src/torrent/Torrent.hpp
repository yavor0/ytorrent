#ifndef TORRENT_TORRENT_H
#define TORRENT_TORRENT_H
#include <string>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <boost/asio.hpp>
#pragma GCC diagnostic pop
#include <TorrentMeta.hpp>
#include "Utils.hpp"

class Torrent
{
private:
    TorrentMeta& tm;
    boost::asio::io_context context;

public:
    Torrent(TorrentMeta& tm);

    void trackerQuery();
};

#endif
