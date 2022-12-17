#ifndef TORRENT_META_H
#define TORRENT_META_H
#include <string>
#include <fstream>
#include <vector>
#include "bencoding/bencoding.h"
#include "Utils.hpp"

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
    UrlMeta announce;
    std::string baseDir;
    std::vector<TorrentFile> files;
    std::string sha1Sums;
    size_t lengthSum;

public:
    TorrentMeta() = default;
    void parseFile(std::string filename);

    void printAll() const;
    inline UrlMeta get_announce() const { return this->announce; }
    inline std::string get_baseDir() const { return this->baseDir; }
    inline size_t get_lengthSum() const { return this->lengthSum; }
    inline std::uint32_t* get_infoHash() { return infoHash; } // non-const
    friend class Torrent;
};

#endif
