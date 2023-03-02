#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "connection.hpp"

class Acceptor {
	typedef std::function<void (const std::shared_ptr<Connection> &conn)> AcceptCallBack;
public:
	Acceptor(uint16_t port);
	~Acceptor();

	void initiateAsyncAcceptLoop(const AcceptCallBack &acceptCB);
	bool isStarted() const;

private:
	std::shared_ptr<asio::io_context> io_context_ptr;
	bool started; 
	asio::ip::tcp::acceptor tcpAcceptor;
};

#endif
