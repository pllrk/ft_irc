#ifndef Server_HPP
# define Server_HPP

#include <string>
#include <vector>
#include <poll.h>

class Server
{
private:
	int _port;
	int _serverFd;
	std::string _password;
	void _eventloop();
	void _handleClientData(std::vector<struct pollfd> &pollfds);

public:
	Server(int port, const std::string &password);
	~Server();

	void run();
};

#endif