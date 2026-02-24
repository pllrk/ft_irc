#ifndef Server_HPP
# define Server_HPP

#include <string>

class Server
{
private:
	int _port;
	int _serverFd;
	std::string _password;

public:
	Server(int port, const std::string &password);
	~Server();

	void run();
};

#endif