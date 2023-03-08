#include <torrent/torrent.hpp>
#include <net/connection.hpp>
#include <utils/utils.hpp>

#include <thread>
#include <functional>
#include <iostream>

#include <boost/program_options.hpp>
#include <stdlib.h>

std::ofstream logFile; // extern

static void print_help(const char *p)
{
	std::clog << "Usage: " << p << " <torrent> {options}" << std::endl;
	std::clog << "\t\t--help (-h)          Print this help message." << std::endl;
	std::clog << "\t\t--version (-v)       Print version." << std::endl;
	std::clog << "\t\t--log (-l)           Specify log filename (defaults to log.txt)." << std::endl;
	std::clog << "\t\t--torrent (-t)       Specify torrent file." << std::endl;
	std::clog << "\t\t--dir (-d)           Specify downloads directory." << std::endl;
	std::clog << "\t\t--seed (-s)          Seed after download has finished." << std::endl;
	std::clog << "Example: " << p << " -t a.torrent -d Torrents -s" << std::endl;
	// std::clog << "\t\t--piecesize (-s)		Specify piece size in KB, this will be rounded to the nearest power of two. Default is 16 KB." << std::endl;
}

int main(int argc, char **argv)
{
	namespace po = boost::program_options;
	bool seed = false;
	uint16_t startport = 63333;
	std::string dir = "Torrents";
	std::string filePath;
	std::string logFname = "out.txt";
	// https://www.boost.org/doc/libs/1_71_0/doc/html/program_options/tutorial.html
	po::options_description opts;
	opts.add_options()("help,h", "print this help message")("version,v", "print version")("log,l", po::value(&logFname), "specify log filename (defaults to log.txt)")("torrent,t", po::value(&filePath), "specify torrent file")("dir,d", po::value(&dir), "specify downloads directory")("seed,s", po::bool_switch(&seed), "seed after download has finished");

	if (argc == 1)
	{
		print_help(argv[0]);
		return 1;
	}

	po::variables_map vm;
	try
	{
		po::store(po::parse_command_line(argc, argv, opts), vm);
		po::notify(vm);
	}
	catch (const std::exception &e)
	{
		std::cerr << argv[0] << ": error parsing command line arguments: " << e.what() << std::endl;
		print_help(argv[0]);
		return 1;
	}
	if (vm.count("help"))
	{
		print_help(argv[0]);
		return 0;
	}

	if (vm.count("version"))
	{
		std::clog << "YTorrent version 0 (in alpha)" << std::endl;
		return 0;
	}
	
	if (vm.count("torrent") == 0)
	{
		std::cerr << argv[0] << ": no torrent file specified" << std::endl;
		print_help(argv[0]);
		return 1;
	}
	logFile.open(logFname, std::ios::out | std::ios::trunc);
	if (!logFile.is_open())
	{
		std::cerr << argv[0] << ": error opening log file: " << logFname << std::endl;
		return 1;
	}


	size_t completed = 0;
	size_t errors = 0;

	Torrent *t = new Torrent();
	if (t->parseFile(filePath, dir) != ParseResult::SUCCESS)
	{
		delete t;
		return 1;
	}
	std::thread runnerThread = std::thread([]()
										   { Connection::start(); });

	std::clog << t->getName() << ": Total size: " << bytesToHumanReadable(t->getTotalSize(), true) << std::endl;
	Torrent::DownloadError error = t->download(startport++);
	switch (error)
	{
	case Torrent::DownloadError::COMPLETED:
		std::clog << t->getName() << ": ---DOWNLOAD FINISHED---" << std::endl;
		++completed;
		break;
	case Torrent::DownloadError::NETWORK_ERROR:
		std::clog << t->getName() << ": Network error was encountered, check your internet connection" << std::endl;
		++errors;
		break;
	case Torrent::DownloadError::TRACKER_QUERY_FAILURE:
		std::clog << t->getName() << ": The tracker has failed to respond in time or some internal error has occured" << std::endl;
		++errors;
		break;
	}
	if (error != Torrent::DownloadError::COMPLETED)
	{
		Connection::stop();
		runnerThread.join();		
		delete t;
		return 1;
	}

	std::clog << t->getName() << ": Downloaded: " << bytesToHumanReadable(t->getDownloadedBytes(), true) << std::endl;
	std::clog << t->getName() << ": Uploaded:   " << bytesToHumanReadable(t->getUploadedBytes(), true) << std::endl;

	if (seed)
	{
		std::clog << t->getName() << "---INITIATING SEEDING---" << std::endl;
		t->seed(startport);
	}

	Connection::stop();
	runnerThread.join(); // https://stackoverflow.com/a/7989043/18301773
	delete t;
	return 0;
}
