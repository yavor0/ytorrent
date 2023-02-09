#include "connection.h"

asio::io_service g_service;
static std::mutex g_connectionLock;
static std::list<std::shared_ptr<asio::streambuf>> g_outputStreams;

Connection::Connection() :
	m_delayedWriteTimer(g_service),
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
		std::bind(&Connection::handleResolve, this, std::placeholders::_1, std::placeholders::_2)
	);
}
void Connection::handleResolve(
	const boost::system::error_code &e,
	asio::ip::basic_resolver<asio::ip::tcp>::iterator endpoint
)
{
	if (e)
		return handleError(e);

	m_socket.async_connect(*endpoint, std::bind(&Connection::handleConnect, this, std::placeholders::_1));
}



void Connection::handleConnect(const boost::system::error_code &e)
{
	if (e)
		return handleError(e);
	else if (m_cb)
		m_cb();
}

