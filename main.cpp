#include <torrent/torrent.hpp>
#include <net/connection.hpp>
#include <utils/utils.hpp>

#include <thread>
#include <functional>
#include <iostream>

#include <boost/program_options.hpp>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int startport = 6881;
	std::string dldir = "Torrents";
	std::string file(argv[1]);
	std::unique_ptr<Torrent> t = std::make_unique<Torrent>();
	int completed = 0;

	if (!t->parseFile(file, dldir))
	{
		std::clog << file << ": corrupted torrent file" << std::endl;
		return 0;
	}
	std::clog << t->getName() << ": Total size: " << bytesToHumanReadable(t->getTotalSize(), true) << std::endl;

	std::thread runnerThread = std::thread([](){Connection::start();});
	runnerThread.detach(); // !!!! https://stackoverflow.com/a/7989043/18301773 !!!!

	Torrent::DownloadError error = t->download(startport++);
	std::clog << t->getName() << ": Downloaded: " << bytesToHumanReadable(t->getDownloadedBytes(), true) << std::endl;
	std::clog << t->getName() << ": Uploaded:   " << bytesToHumanReadable(t->getUploadedBytes(), true) << std::endl;
	Connection::stop();
	return 0;
}

