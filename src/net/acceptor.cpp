#include "acceptor.hpp"

Acceptor::Acceptor(uint16_t port)
	: tcpAcceptor(g_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
	tcpAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true)); // https://prnt.sc/F8LJGCuHN8th
}

Acceptor::~Acceptor()
{
	tcpAcceptor.cancel();
	tcpAcceptor.close();
}

void Acceptor::accept(const AcceptCallBack &acceptCB)
{
	std::shared_ptr<Connection> conn = std::make_shared<Connection>(); 
	tcpAcceptor.async_accept(conn->socket,
		[=] (const boost::system::error_code &error) {
			if (!error)
			{
				return acceptCB(conn);
			}
		}
	);
}

