#ifndef PEER_H
#define PEER_H
#include <string>


class Torrent;
class Peer
{
private:
    std::string ip;
    std::string port;
    Torrent *torr;
    
public:
    Peer(Torrent *t) : torr(t){};
    void connect();
};

#endif
