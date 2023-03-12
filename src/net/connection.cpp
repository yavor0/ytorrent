#include "connection.hpp"
#include <iostream> // remove

asio::io_context g_io_context;
boost::asio::executor_work_guard<boost::asio::io_context::executor_type> g_work_guard(g_io_context.get_executor());

Connection::Connection() : 
						   resolver(g_io_context),
						   socket(g_io_context)
{
}

Connection::~Connection()
{
	close();
}

void Connection::start()
{
	g_io_context.run();
}

void Connection::stop()
{
	g_work_guard.reset();
	g_io_context.stop();
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
				me->handleError(e);
				return;
			}
			// Connect handler
			me->socket.async_connect(
				*endpoint,
				[me](const boost::system::error_code &e)
				{
					if (e)
					{
						me->handleError(e);
						return;
					}
					else if (me->connCB)
					{
						me->connCB();
					}
				});
		});
}

void Connection::close()
{
	if (!isConnected())
	{
		return;
	}
	// due to still unknown to me reasons when on graceful disconnect aka. .shutdown is called sometimes an error is thrown 
	boost::system::error_code ec;
	this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec); // https://stackoverflow.com/a/3068106/18301773
	this->socket.close();
}

void Connection::read(size_t bytes, const ReadCallback &rc) // bruh https://stackoverflow.com/q/291871/18301773
{
	if (!isConnected())
	{
		return;
	}

	this->readCB = rc;
	asio::async_read( // remember: https://stackoverflow.com/a/28931673/18301773
		this->socket, this->inputStream.prepare(bytes),
		// Read handler
		[me = shared_from_this()](const boost::system::error_code &e, size_t readSize)
		{
			me->inputStream.commit(readSize); // https://dens.website/tutorials/cpp-asio/dynamic-buffers-2
			if (e)
			{
				me->handleError(e);
				return;
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
		return asio::detail::socket_ops::host_to_network_long(ip.address().to_v4().to_ulong());
	}

	return 0;
}