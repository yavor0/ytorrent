#include "connection.hpp"

asio::io_service g_service;
static std::mutex g_connectionLock;
static std::list<std::shared_ptr<asio::streambuf>> g_outputStreams;

Connection::Connection() : m_delayedWriteTimer(g_service),
						   m_resolver(g_service),
						   m_socket(g_service)
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

	m_cb = cb;
	m_resolver.async_resolve(
		query,
		std::bind(&Connection::handleResolve, this, std::placeholders::_1, std::placeholders::_2));
}
void Connection::handleResolve(const boost::system::error_code &e,
							   asio::ip::basic_resolver<asio::ip::tcp>::iterator endpoint)
{
	if (e)
		return handleError(e);

	m_socket.async_connect(*endpoint, std::bind(&Connection::handleConnect, this, std::placeholders::_1));
}

void Connection::write(const uint8_t *bytes, size_t size)
{
	if (!isConnected())
		return;

	if (!m_outputStream)
	{
		g_connectionLock.lock();
		if (!g_outputStreams.empty())
		{
			m_outputStream = g_outputStreams.front();
			g_outputStreams.pop_front();
		}
		else
			m_outputStream = std::shared_ptr<asio::streambuf>(new asio::streambuf);
		g_connectionLock.unlock();

		m_delayedWriteTimer.cancel();
		m_delayedWriteTimer.expires_from_now(boost::posix_time::milliseconds(10));
		m_delayedWriteTimer.async_wait(std::bind(&Connection::internalWrite, this, std::placeholders::_1));
	}

	std::ostream os(m_outputStream.get());
	os.write((const char *)bytes, size);
	os.flush();
}

void Connection::internalWrite(const boost::system::error_code &e)
{
	if (e == asio::error::operation_aborted)
		return;

	std::shared_ptr<asio::streambuf> outputStream = m_outputStream;
	m_outputStream = nullptr;

	asio::async_write(
		m_socket, *outputStream,
		std::bind(&Connection::handleWrite, this, std::placeholders::_1, std::placeholders::_2, outputStream));
}
void Connection::handleWrite(const boost::system::error_code &e, size_t bytes, std::shared_ptr<asio::streambuf> outputStream)
{
	m_delayedWriteTimer.cancel();
	if (e == asio::error::operation_aborted)
		return;

	outputStream->consume(outputStream->size());
	g_outputStreams.push_back(outputStream);
	if (e)
		handleError(e);
}

void Connection::read_partial(size_t bytes, const ReadCallback &rc)
{
	if (!isConnected())
		return;

	m_rc = rc;
	m_socket.async_read_some(
		asio::buffer(m_inputStream.prepare(bytes)),
		std::bind(&Connection::handleRead, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::handleConnect(const boost::system::error_code &e)
{
	if (e)
		return handleError(e);
	else if (m_cb)
		m_cb();
}
