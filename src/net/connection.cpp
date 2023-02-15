#include "connection.hpp"

asio::io_context g_io_context;
boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(g_io_context.get_executor());

Connection::Connection() : resolver(g_io_context),
						   socket(g_io_context)
{
}

Connection::~Connection()
{
	close(false);
}

void Connection::start()
{
	g_io_context.run();
}

void Connection::stop()
{
	work_guard.reset(); // Work guard is destroyed, io_context::run is free to return
}

void Connection::connect(const std::string &host, const std::string &port, const ConnectCallback &cb)
{
	asio::ip::tcp::resolver::query query(host, port);

	this->connCB = cb;
	this->resolver.async_resolve(
		query,
		// Resolve handler
		[this](const boost::system::error_code &e, asio::ip::basic_resolver<asio::ip::tcp>::iterator endpoint)
		{
			if (e)
			{
				return handleError(e);
			}
			// Connect handler
			this->socket.async_connect(
				*endpoint,
				[this](const boost::system::error_code &e)
				{
					if (e)
					{
						return handleError(e);
					}
					else if (this->connCB)
					{
						this->connCB();
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

	boost::system::error_code ec;
	this->socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	if (ec && warn && this->errorCB)
	{
		this->errorCB(ec.message());
	}

	this->socket.close();
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
		// Read handler
		[this](const boost::system::error_code &e, size_t readSize)
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
		});
}

void Connection::write(const uint8_t *data, size_t bytes)
{
	if (!isConnected())
		return;
	asio::async_write(
		this->socket,
		asio::buffer(data, bytes),
		[this](const boost::system::error_code &error, std::size_t bytesTransferred)
		{
			if (error == asio::error::operation_aborted)
				return;
			if (error)
			{
				handleError(error);
			}
		});
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