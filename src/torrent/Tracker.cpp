#include "Tracker.hpp"
#include <iostream>

Tracker::Tracker(std::string url)
    : url(url)
{
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
    boost::asio::ip::tcp::iostream s;

    // The entire sequence of I/O operations must complete within 60 seconds.
    // If an expiry occurs, the socket is automatically closed and the stream
    // becomes bad.
    s.expires_after(std::chrono::seconds(60));

    // Establish a connection to the server.
    s.connect(this->url, "http");
    if (!s)
    {
        std::cout << "Unable to connect: " << s.error().message() << "\n";
        return;
    }

    // s << "GET /announce?"
    //   << "test"
    //   << " HTTP/1.0\r\n";
    // s << "Connection: close\r\n\r\n";

    s << "GET /announce?" << "info_hash=" << "99c82bb73505a3c0b453f9fae881d6e5a32a0c1" << "&compact=1"
    << "&peer_id=" << "-YT00000" << "&left=" << 4071903232 << " HTTP/1.0\r\n"
    << "\r\n";

    // Check that response is OK.
    std::string http_version;
    s >> http_version;
    unsigned int status_code;
    s >> status_code;
    std::string status_message;
    std::getline(s, status_message);
    if (!s || http_version.substr(0, 5) != "HTTP/")
    {
        std::cout << "Invalid response\n";
        return;
    }
    // if (status_code != 200)
    // {
    //     std::cout << "Response returned with status code " << status_code << "\n";
    //     return;
    // }

    // Process the response headers, which are terminated by a blank line.
    std::string header;
    while (std::getline(s, header) && header != "\r")
        std::cout << header << "\n";
    std::cout << "\n";

    // Write the remaining data to output.
    std::cout << s.rdbuf();

}


