#ifndef TORRENT_META_H
#define TORRENT_META_H
#include <string>
#include <fstream>
#include <vector>


// https://stackoverflow.com/a/40020484/18301773
// enable_shared_from_this requires public inheritence otherwise bad_weak_ptr is thrown from Bitem
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor" 
#include "bencoding/bencoding.h"
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <boost/uuid/detail/sha1.hpp>
#pragma GCC diagnostic pop

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
    size_t lengthSum = 0;

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
