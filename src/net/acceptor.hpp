#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "connection.hpp"

class Acceptor {
	typedef std::function<void (const std::shared_ptr<Connection> &conn)> AcceptCallBack;
public:
	Acceptor(uint16_t port);
	~Acceptor();

	void accept(const AcceptCallBack &acceptCB);

private:
	asio::ip::tcp::acceptor tcpAcceptor;
};

#endif
