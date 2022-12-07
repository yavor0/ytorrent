#ifndef __TORRENT_H
#define __TORRENT_H
#include <string>
#include <fstream>
#include <vector>
#include "bencoding/bencoding.h"

struct TorrentFile{
    std::string name;
    std::string path;
    size_t length;
};



class TorrentMeta
{
private:
    std::string announce;
    std::string baseDir;
    std::vector<TorrentFile> files;

public:
    TorrentMeta() = default;
    void parseFile(std::string filename);
    std::string getAnnounce() const;
    void printAll() const;
};

#endif
