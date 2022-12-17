#ifndef TORRENT_TORRENT_H
#define TORRENT_TORRENT_H
#include <string>
#include <boost/asio.hpp>
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
