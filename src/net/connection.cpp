#include "connection.hpp"

std::shared_ptr<asio::io_context> g_io_context = std::make_shared<asio::io_context>();
boost::asio::executor_work_guard<boost::asio::io_context::executor_type> g_work_guard((*g_io_context).get_executor());
// https://www.reddit.com/r/cpp/comments/jdy2gd/comment/g9bbflz/?utm_source=share&utm_medium=web2x&context=3
Connection::Connection() : io_context_ptr(g_io_context),
						   resolver(*io_context_ptr),
						   socket(*io_context_ptr)
{
}
Connection::~Connection()
{
	close(false);
}

void Connection::start()
{
	std::shared_ptr<asio::io_context> local_io_context_ptr = g_io_context;
	(*local_io_context_ptr).run();
}

void Connection::stop()
{
	// g_io_context.stop(); // https://stackoverflow.com/a/18555384/18301773
	g_work_guard.reset();
}

void Connection::connect(const std::string &host, const std::string &port, const ConnectCallback &cb)
{
	asio::ip::tcp::resolver::query query(host, port);

	this->connCB = cb;
	this->resolver.async_resolve(
		query,
		// Resolve handler
		[me = shared_from_this()](const boost::system::error_code &e, asio::ip::basic_resolver<asio::ip::tcp>::iterator endpoint)
		{
			if (e)
			{
				return me->handleError(e);
			}
			// Connect handler
			me->socket.async_connect(
				*endpoint,
				[me](const boost::system::error_code &e)
				{
					if (e)
					{
						return me->handleError(e);
					}
					else if (me->connCB)
					{
						me->connCB();
					}
				});
		});
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
	this->socket.close();
	boost::system::error_code ec;
	this->socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	if (ec && warn && this->errorCB)
	{
		this->errorCB(ec.message());
	}
}

void Connection::read(size_t bytes, const ReadCallback &rc) // bruh https://stackoverflow.com/q/291871/18301773
{
	if (!isConnected())
	{
		return;
	}

	this->readCB = rc;
	asio::async_read(
		this->socket, this->inputStream.prepare(bytes),
		// Read handler
		[me = shared_from_this()](const boost::system::error_code &e, size_t readSize)
		{
			if (e)
			{
				return me->handleError(e);
			}

			if (me->readCB)
			{
				const uint8_t *data = asio::buffer_cast<const uint8_t *>(me->inputStream.data());
				me->readCB(data, readSize);
			}

			me->inputStream.consume(readSize);
		});
}

void Connection::write(const uint8_t *data, size_t bytes)
{
	if (!isConnected())
		return;
	asio::async_write(
		this->socket,
		asio::buffer(data, bytes),
		[me = shared_from_this()](const boost::system::error_code &error, std::size_t bytesTransferred)
		{
			if (error)
			{
				me->handleError(error);
			}
		});
}

void Connection::handleError(const boost::system::error_code &error)
{
	// check for operation aborted!!! https://www.reddit.com/r/cpp/comments/jdy2gd/asio_users_how_do_you_deal_with_cancellation/
	if (error == asio::error::operation_aborted)
	{
		return;
	}
	if (this->errorCB)
	{
		this->errorCB(error.message());
	}
	if (isConnected()) // User is free to close the connection before me
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
		return ip.address().to_v4().to_ulong();
	}

	return 0;
}