#include "Tracker.hpp"
#include <Utils.hpp>


#include <iostream>


Tracker::Tracker(TorrentMeta& tm)
    :tm(tm)
{
    std::string url = tm.get_announce();
    if (url.find("https://") == 0)
    {
        std::string parsed = url.substr(8);
        this->url = parsed;
    }
    else if (url.find("http://") == 0)
    {
        std::string parsed = url.substr(7);
        this->url = parsed;
    }
    else
    {
        std::cout << "Invalid URL" << std::endl;
    }
    this->port = "80";
}

void Tracker::query()
{


}


