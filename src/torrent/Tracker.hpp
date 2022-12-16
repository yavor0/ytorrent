#ifndef __TORRENT_TRACKER_H
#define __TORRENT_TRACKER_H
#include <string>
#include <boost/asio.hpp>


class Tracker{
private:
    std::string url;
    std::string port;
    boost::asio::io_context context;
public:
    Tracker(std::string url);
    inline std::string get_url() const { return this->url; }
    inline std::string get_port() const { return this->port; }
    
    void query();
};

#endif
