#include "acceptor.hpp"

Acceptor::Acceptor(uint16_t port)
	: started(false),
	  tcpAcceptor(g_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))

{
	tcpAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true)); // https://prnt.sc/F8LJGCuHN8th
}

Acceptor::~Acceptor()
{
	tcpAcceptor.cancel();
	tcpAcceptor.close();
}

bool Acceptor::isStarted() const
{
	return started;
}

void Acceptor::initiateAsyncAcceptLoop(const AcceptCallBack &acceptCB)
{
	started = true;
	std::shared_ptr<Connection> conn = std::make_shared<Connection>();
	tcpAcceptor.async_accept(conn->socket,
							 [acceptCB, conn, this](const boost::system::error_code &error)
							 {
								 if (!error)
								 {
									 acceptCB(conn);
									 initiateAsyncAcceptLoop(acceptCB);
								 }
							 });
}
