#ifndef __TORRENT_META_H
#define __TORRENT_META_H
#include <string>
#include <fstream>
#include <vector>
#include "bencoding/bencoding.h"

struct TorrentFile
{
    std::string name;
    std::string path;
    size_t length;
};

class TorrentMeta
{
private:
    std::uint32_t infoHash[5] = {0};
    std::string announce;
    std::string baseDir;
    std::vector<TorrentFile> files;
    std::string sha1Sums;
    size_t lengthSum;

public:
    TorrentMeta() = default;
    void parseFile(std::string filename);
    std::string getAnnounce() const;
    void printAll() const;
};

#endif
