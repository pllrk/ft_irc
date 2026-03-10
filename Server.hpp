#ifndef Server_HPP
# define Server_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
private:
	int _port;
	int _serverFd;
	std::string _password;

	std::map<int, Client*> _clients;        // fd -> Client object
	std::map<std::string, int> _nickToFd;   // nickname -> fd (for uniqueness check)
	std::map<std::string, Channel*> _channels;  // channel name -> Channel object

	void _eventloop();
	void _handleClientData(std::vector<struct pollfd> &pollfds);

	void _handleCommand(Client &client, const std::string &line);
	void _cmdPass(Client &client, const std::vector<std::string> &params);
	void _cmdNick(Client &client, const std::vector<std::string> &params);
	void _cmdUser(Client &client, const std::vector<std::string> &params);
	void _cmdJoin(Client &client, const std::vector<std::string> &params);
	void _cmdPart(Client &client, const std::vector<std::string> &params);
	void _cmdPrivMsg(Client &client, const std::vector<std::string> &params, const std::string &msg);
	void _cmdQuit(Client &client, const std::vector<std::string> &params);
	void _cmdPing(Client &client, const std::vector<std::string> &params);
	void _tryRegister(Client &client);
	void _handleClientDisconnect(Client &client);

	std::vector<std::string> _splitIrcParams(const std::string &rest) const;
	std::string _toUpper(const std::string &s) const;
	void _sendMsg(int fd, const std::string &msg) const;

public:
	Server(int port, const std::string &password);
	~Server();

	void run();
};

#endif