#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <utils/utils.hpp>

#include <list>
#include "outgoingmessage.hpp"

namespace asio = boost::asio;
class Connection : public std::enable_shared_from_this<Connection> // https://stackoverflow.com/a/2795371/18301773
{
typedef std::function<void(const uint8_t *readCB, size_t size)> ReadCallback;
typedef std::function<void()> ConnectCallback;
typedef std::function<void(const std::string &errorCB)> ErrorCallback;

private:
	asio::ip::tcp::resolver resolver;
	asio::ip::tcp::socket socket;

	ReadCallback readCB;
	ConnectCallback connCB;
	ErrorCallback errorCB;

	asio::streambuf inputStream;

	void handleError(const boost::system::error_code &e);
	friend class Acceptor;
public:
	Connection();
	~Connection();

	static void start();
	static void stop();

	void connect(const std::string &host, const std::string &port, const ConnectCallback &connCB);
	void close(bool warn = true); // Pass false in ErrorCallback otherwise possible infinite recursion
	bool isConnected() const { return this->socket.is_open(); }

	inline void write(const OutgoingMessage &om) { write(om.data(0), om.size()); }
	void write(const uint8_t *data, size_t bytes);
	void read(size_t bytes, const ReadCallback &readCB);

	std::string getIPString() const { return parseIp(getIP()); }
	uint32_t getIP() const;

	void setErrorCallback(const ErrorCallback &errorCB) { this->errorCB = errorCB; }
};
extern asio::io_context g_io_context;
extern boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

#endif
