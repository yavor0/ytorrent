#ifndef TORRENT_TRACKER_H
#define TORRENT_TRACKER_H
#include <string>
#include <boost/asio.hpp>
#include <TorrentMeta.hpp>

class Tracker
{
private:
    TorrentMeta& tm;
    std::string url;
    std::string port;
    boost::asio::io_context context;
public:
    Tracker(TorrentMeta& tm);
    inline std::string get_url() const { return this->url; }
    inline std::string get_port() const { return this->port; }
    
    void query();
};

#endif
