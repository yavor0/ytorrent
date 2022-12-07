#include "TorrentMeta.h"
#include <iostream> // REMOVE
#include <typeinfo>
using namespace bencoding;

void TorrentMeta::parseFile(std::string filename)
{ // ADD ERROR HANDLING

    std::shared_ptr<BItem> decodedData;
    std::ifstream input(filename);
    decodedData = decode(input);
    auto data = decodedData->as<BDictionary>();

    // extract announce
    std::shared_ptr<BItem> &annVal = (*data)[BString::create("announce")];
    std::shared_ptr<BString> bStrAnn = annVal->as<BString>();
    this->announce = bStrAnn->value();

    std::shared_ptr<BItem> &infoVal = (*data)[BString::create("info")];
    std::shared_ptr<BDictionary> infoDict = infoVal->as<BDictionary>();

    std::shared_ptr<BItem> &files = (*infoDict)[BString::create("files")];
    if (files != nullptr)
    {
        this->baseDir = ((*infoDict)[BString::create("name")])->as<BString>()->value();

        // extract each file in this->files
        std::shared_ptr<BList> bListFiles = files->as<BList>();
        for (std::shared_ptr<BItem> file : *bListFiles)
        {
            TorrentFile tf;
            std::shared_ptr<BDictionary> bFile = file->as<BDictionary>();

            std::shared_ptr<BInteger> bFileLength = ((*bFile)[BString::create("length")])->as<BInteger>();
            tf.length = bFileLength->value();

            std::shared_ptr<BList> bFilePath = ((*bFile)[BString::create("path")])->as<BList>();
            for (BList::iterator it = bFilePath->begin(); it != bFilePath->end(); ++it)
            {
                it++;
                if (it == bFilePath->end())
                {
                    it--;
                    tf.name = it->get()->as<BString>()->value();
                    break;
                }
                it--;

                std::string currDir = it->get()->as<BString>()->value();
                tf.path += "/" + currDir;
            }
            this->files.push_back(tf);
        }
    }
    else
    {
        TorrentFile tf;
        tf.name = ((*infoDict)[BString::create("name")])->as<BString>()->value();
        tf.length = ((*infoDict)[BString::create("length")])->as<BInteger>()->value();
        this->files.push_back(tf);
    }

    // sha1sums

    // TorrentFile f;
    // std::shared_ptr<bencoding::BItem> bItemLen = (*infoDict)[bencoding::BString::create("length")];
    // std::shared_ptr<bencoding::BInteger> bIntLen = bItemLen->as<bencoding::BInteger>();

    // std::shared_ptr<bencoding::BItem> bItemName = (*infoDict)[bencoding::BString::create("name")];
    // std::shared_ptr<bencoding::BString> bStrName = bItemName->as<bencoding::BString>();

    // f.length = bIntLen->value();
    // f.name = bStrName->value();
    // files.push_back(f);
}

std::string TorrentMeta::getAnnounce() const
{
    return this->announce;
}

void TorrentMeta::printAll() const
{
    std::cout << "Announce: " << announce << std::endl;
    std::cout << "Files: " << files.size() << std::endl;
    std::cout << "Base directory: " << baseDir << std::endl;
    for (auto &file : files)
    {
        std::cout << "File name: " << file.name << std::endl;
        std::cout << "File length: " << file.length << std::endl;
        std::cout << "File path: " << file.path << std::endl;
        std::cout << std::endl;
    }
}
