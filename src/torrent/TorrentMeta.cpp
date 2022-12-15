#include "TorrentMeta.h"
#include <iostream> // REMOVE
#include <typeinfo>
#include <boost/uuid/detail/sha1.hpp>

using namespace bencoding;

void TorrentMeta::parseFile(std::string filename)
{ // ADD ERROR HANDLING

    std::shared_ptr<BItem> decodedData;
    std::ifstream input(filename);
    decodedData = bencoding::decode(input);
    std::shared_ptr<BDictionary> torrMetaDict = decodedData->as<BDictionary>();

    // extract announce
    std::shared_ptr<BItem> &annVal = (*torrMetaDict)[BString::create("announce")]; //
    std::shared_ptr<BString> bStrAnn = annVal->as<BString>();
    this->announce = bStrAnn->value();

    std::shared_ptr<BItem> &infoVal = (*torrMetaDict)[BString::create("info")];
    std::shared_ptr<BDictionary> infoDict = infoVal->as<BDictionary>();

    // infoHash
    std::string strInfoHash = bencoding::encode(infoDict);

    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(strInfoHash.data(), strInfoHash.size());
    sha1.get_digest(this->infoHash);

    // files
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
                // it++;
                if (std::next(it) == bFilePath->end())
                {
                    // it--;
                    tf.name = it->get()->as<BString>()->value();
                    break;
                }
                // it--;

                std::string currDir = it->get()->as<BString>()->value();
                tf.path += "/" + currDir;
            }
            this->files.push_back(tf);
            this->lengthSum += tf.length;
        }
    }
    else
    {
        TorrentFile tf;
        tf.name = ((*infoDict)[BString::create("name")])->as<BString>()->value();
        tf.length = ((*infoDict)[BString::create("length")])->as<BInteger>()->value();
        this->files.push_back(tf);
        this->lengthSum += tf.length;
    }

    // sha1Sums: to be improved
    this->sha1Sums = ((*infoDict)[BString::create("pieces")])->as<BString>()->value();
}

std::string TorrentMeta::getAnnounce() const
{
    return this->announce;
}

void TorrentMeta::printAll() const
{
    std::ios_base::fmtflags f(std::cout.flags()); // used to reset the cout flags after std::hex is used
    std::cout << "Announce: " << announce << std::endl;
    
    std::cout << "Info hash: ";
    for (int i = 0; i < 5; i++)
    {
        std::cout << std::hex << (this->infoHash)[i];
    }
    std::cout << std::endl;
    std::cout.flags(f);
    std::cout << "Length: " << this->lengthSum << std::endl;
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
