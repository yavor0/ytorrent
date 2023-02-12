#include "connection.hpp"

asio::io_service g_service;
static std::mutex connectionLock;
static std::list<std::shared_ptr<asio::streambuf>> outputStreams;

Connection::Connection() : delayedWriteTimer(g_service),
						   resolver(g_service),
						   socket(g_service)
{
}

Connection::~Connection()
{
	close(false);
}

void Connection::poll()
{
	g_service.reset();
	g_service.poll();
}

void Connection::connect(const std::string &host, const std::string &port, const ConnectCallback &cb)
{
	asio::ip::tcp::resolver::query query(host, port);

	this->connCB = cb;
	this->resolver.async_resolve(
		query,
		std::bind(&Connection::handleResolve, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::close(bool warn)
{
	if (!isConnected())
	{
		if (this->errorCB && warn)
		{
			this->errorCB("Connection::close(): Called on an already closed connection!");
		}
		return;
	}

	boost::system::error_code ec;
	this->socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	if (ec && warn && this->errorCB)
	{
		this->errorCB(ec.message());
	}

	this->socket.close();
}

void Connection::write(const uint8_t *bytes, size_t size)
{
	if (!isConnected())
		return;

	if (!this->outputStream)
	{
		connectionLock.lock();
		if (!outputStreams.empty())
		{
			this->outputStream = outputStreams.front();
			outputStreams.pop_front();
		}
		else
		{
			this->outputStream = std::shared_ptr<asio::streambuf>(new asio::streambuf);
		}

		connectionLock.unlock();

		this->delayedWriteTimer.cancel();
		this->delayedWriteTimer.expires_from_now(boost::posix_time::milliseconds(10));
		this->delayedWriteTimer.async_wait(std::bind(&Connection::internalWrite, this, std::placeholders::_1));
	}

	std::ostream os(this->outputStream.get());
	os.write((const char *)bytes, size);
	os.flush();
}

void Connection::read(size_t bytes, const ReadCallback &rc)
{
	if (!isConnected())
	{
		return;
	}

	this->readCB = rc;
	asio::async_read(
		this->socket, asio::buffer(this->inputStream.prepare(bytes)),
		std::bind(&Connection::handleRead, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::internalWrite(const boost::system::error_code &e)
{
	if (e == asio::error::operation_aborted)
	{
		return;
	}

	std::shared_ptr<asio::streambuf> outputStream = this->outputStream;
	this->outputStream = nullptr;

	asio::async_write(
		this->socket, *outputStream,
		std::bind(&Connection::handleWrite, this, std::placeholders::_1, std::placeholders::_2, outputStream));
}

void Connection::handleWrite(const boost::system::error_code &e, size_t bytes, std::shared_ptr<asio::streambuf> outputStream)
{
	this->delayedWriteTimer.cancel();
	if (e == asio::error::operation_aborted)
	{
		return;
	}

	outputStream->consume(outputStream->size());
	outputStreams.push_back(outputStream);
	if (e)
	{
		handleError(e);
	}
}

void Connection::handleRead(const boost::system::error_code &e, size_t readSize)
{
	if (e)
	{
		return handleError(e);
	}

	if (this->readCB)
	{
		const uint8_t *data = asio::buffer_cast<const uint8_t *>(this->inputStream.data());
		this->readCB(data, readSize);
	}

	this->inputStream.consume(readSize);
}

void Connection::handleResolve(
	const boost::system::error_code &e,
	asio::ip::basic_resolver<asio::ip::tcp>::iterator endpoint)
{
	if (e)
	{
		return handleError(e);
	}

	this->socket.async_connect(*endpoint, std::bind(&Connection::handleConnect, this, std::placeholders::_1));
}

void Connection::handleConnect(const boost::system::error_code &e)
{
	if (e)
	{
		return handleError(e);
	}
	else if (this->connCB)
	{
		this->connCB();
	}
}

void Connection::handleError(const boost::system::error_code &error)
{
	if (error == asio::error::operation_aborted)
	{
		return;
	}

	if (this->errorCB)
	{
		this->errorCB(error.message());
	}
	if (isConnected()) // User is free to close the connection before us
	{
		close();
	}
}
uint32_t Connection::getIP() const
{
	if (!isConnected())
	{
		return 0;
	}

	boost::system::error_code error;
	const asio::ip::tcp::endpoint ip = this->socket.remote_endpoint(error);
	if (!error)
	{
		return asio::detail::socket_ops::host_to_network_long(ip.address().to_v4().to_ulong());
	}

	return 0;
}