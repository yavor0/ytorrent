# yTorrent

yTorrent is a simple torrent client written in C++14 for learning purposes.

## Installation

### Prerequisites

- Linux operating system distribution, Ubuntu 20.04 LTS is recommended
- A compiler supporting the C++14 standard: for example g++ (>= 9.4.0)
- CMake to automate the compilation and installation process (>= 3.16.0)
- libboost-dev for Boost.Asio and Boost.Program_Options (=1.71.0)

### Compiling the Program

Navigate to the main `ytorrent/` folder and generate the build files using cmake:

```bash
$ cmake -S . -B build/
```

After generating the build files, compile the program:

```bash
$ cd build/
$ make
```

### Optional: Compiling the cpp-bencoding Library

The program comes with the open-source library [cpp-bencoding](https://github.com/s3rvac/cpp-bencoding.git) compiled. If you want to manually compile it, run the following commands from the main (ytorrent/) directory:

```bash
$ mkdir dependencies
$ cd dependencies
$ git clone https://github.com/s3rvac/cpp-bencoding.git
$ cd cpp-bencoding
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

## Usage

### Running the Program

To run the ytorrent executable, navigate to the build folder and use the following command:

```bash
$ ./yTorrent
```

### Command-Line Options

Available options:

- `-h`: Display help information
- `-v`: Display the version
- `-l`: Specify a log file name (default: log.txt)
- `-t`: Specify the path to the torrent file to be downloaded
- `-d`: Specify the directory where the torrent file should be downloaded
- `-s`: Enter seeding mode after the torrent is downloaded

Example usage:

```bash
$ ./yTorrent -t /path/to/torrent/file.torrent -d /path/to/download/folder -s
```

### Downloading and Seeding

The download progress will be displayed, including information such as completed pieces, downloaded and uploaded data, download speed, and estimated time remaining.

Once the download is complete, if the `-s` option was passed, the client will enter seeding mode until the user stops the program (ctrl+c).

## Current Limitations and Future Improvements

The current version of the torrent client has some limitations:

- Does not support torrents with multiple files
- Does not support DHT (Distributed Hash Table)
- Does not support PEX (Peer Exchange)
- Does not support Magnet Links
- Does not support UDP trackers
- Does not support encryption

However, it does support the core basic functionalities:

- Can fully download torrents
- Can seed torrents
- Has a very basic TUI (Text User Interface)
