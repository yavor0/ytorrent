#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <utils/utils.hpp>

#include <mutex>
#include <list>
#include "outgoingmessage.hpp"

namespace asio = boost::asio;
class Connection
{
typedef std::function<void(const uint8_t *readCB, size_t size)> ReadCallback;
typedef std::function<void()> ConnectCallback;
typedef std::function<void(const std::string &errorCB)> ErrorCallback;

private:
	asio::deadline_timer delayedWriteTimer; // rename?
	asio::ip::tcp::resolver resolver;
	asio::ip::tcp::socket socket;

	ReadCallback readCB;
	ConnectCallback connCB;
	ErrorCallback errorCB;

	std::shared_ptr<asio::streambuf> outputStream;
	asio::streambuf inputStream;

	void internalWrite(const boost::system::error_code &e);
	void handleRead(const boost::system::error_code &e, size_t);
	void handleWrite(const boost::system::error_code &e, size_t, std::shared_ptr<asio::streambuf> outputStream);
	void handleConnect(const boost::system::error_code &e);
	void handleResolve(const boost::system::error_code &e, asio::ip::basic_resolver<asio::ip::tcp>::iterator endpoint);
	void handleError(const boost::system::error_code &e);

public:
	Connection();
	~Connection();

	static void poll();

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
extern asio::io_service g_service;

#endif
