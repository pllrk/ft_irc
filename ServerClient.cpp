#include "Server.hpp"
#include <sys/socket.h>
#include <cctype>
#include <unistd.h>

// Parse and route incoming IRC commands
void Server::_handleCommand(Client &client, const std::string &line)
{
	size_t sp = line.find(' ');
	std::string cmd = _toUpper(sp == std::string::npos ? line : line.substr(0, sp));
	std::string rest = (sp == std::string::npos) ? "" : line.substr(sp + 1);
	std::vector<std::string> params = _splitIrcParams(rest);

	if (cmd == "PASS")
		_cmdPass(client, params);
	else if (cmd == "NICK")
		_cmdNick(client, params);
	else if (cmd == "USER")
		_cmdUser(client, params);
	else if (cmd == "JOIN")
		_cmdJoin(client, params);
	else if (cmd == "PART")
		_cmdPart(client, params);
	else if (cmd == "PRIVMSG")
		_cmdPrivMsg(client, params, rest);
	else if (cmd == "QUIT")
		_cmdQuit(client, params);
	else if (cmd == "PING")
		_cmdPing(client, params);
	else
		_sendMsg(client.getFd(), ":ft_irc 421 * " + cmd + " :Unknown command\r\n");
}

// Handle PASS command: Authenticate with server password
void Server::_cmdPass(Client &client, const std::vector<std::string> &params)
{
	if (client.isAuthenticated())
		return _sendMsg(client.getFd(), ":ft_irc 462 * :You may not reregister\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ft_irc 461 * PASS :Not enough parameters\r\n");
	if (params[0] != _password)
		return _sendMsg(client.getFd(), ":ft_irc 464 * :Password incorrect\r\n");

	client.setAuthenticated(true);
	_tryRegister(client);
}

// Handle NICK command: Set or change client nickname
void Server::_cmdNick(Client &client, const std::vector<std::string> &params)
{
	if (!client.isAuthenticated())
		return _sendMsg(client.getFd(), ":ft_irc 464 * :Password required\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ft_irc 431 * :No nickname given\r\n");

	const std::string &newNick = params[0];
	std::map<std::string, int>::iterator it = _nickToFd.find(newNick);
	if (it != _nickToFd.end() && it->second != client.getFd())
		return _sendMsg(client.getFd(), ":ft_irc 433 * " + newNick + " :Nickname is already in use\r\n");

	if (!client.getNickname().empty())
		_nickToFd.erase(client.getNickname());

	client.setNickname(newNick);
	_nickToFd[newNick] = client.getFd();

	_tryRegister(client);
}

// Handle USER command: Set username and realname
void Server::_cmdUser(Client &client, const std::vector<std::string> &params)
{
	if (!client.isAuthenticated())
		return _sendMsg(client.getFd(), ":ft_irc 464 * :Password required\r\n");
	if (client.isRegistered())
		return _sendMsg(client.getFd(), ":ft_irc 462 * :You may not reregister\r\n");
	if (params.size() < 4)
		return _sendMsg(client.getFd(), ":ft_irc 461 * USER :Not enough parameters\r\n");

	client.setUsername(params[0]);
	client.setRealname(params[3]);
	_tryRegister(client);
}

// Check if client is fully registered and send welcome message
void Server::_tryRegister(Client &client)
{
	if (!client.isRegistered()
		&& client.isAuthenticated()
		&& !client.getNickname().empty()
		&& !client.getUsername().empty())
	{
		client.setRegistered(true);
		_sendMsg(client.getFd(), ":ft_irc 001 " + client.getNickname() + " :Welcome to ft_irc\r\n");
	}
}

// Convert string to uppercase
std::string Server::_toUpper(const std::string &s) const
{
	std::string out = s;
	for (size_t i = 0; i < out.size(); ++i)
		out[i] = static_cast<char>(std::toupper(out[i]));
	return out;
}

// Split IRC command parameters (handles :trailing param properly)
std::vector<std::string> Server::_splitIrcParams(const std::string &rest) const
{
	std::vector<std::string> p;
	size_t i = 0;

	while (i < rest.size())
	{
		while (i < rest.size() && rest[i] == ' ')
			++i;
		if (i >= rest.size())
			break;

		if (rest[i] == ':')
		{
			p.push_back(rest.substr(i + 1));
			break;
		}

		size_t j = i;
		while (j < rest.size() && rest[j] != ' ')
			++j;
		p.push_back(rest.substr(i, j - i));
		i = j;
	}
	return p;
}

// Send a message to a client
void Server::_sendMsg(int fd, const std::string &msg) const
{
	send(fd, msg.c_str(), msg.size(), 0);
}

// Handle QUIT command: Disconnect gracefully
void Server::_cmdQuit(Client &client, const std::vector<std::string> &params)
{
	std::string quitMsg = "QUIT";
	if (!params.empty())
		quitMsg += " :" + params[0];
	
	// Notify all channels before closing
	_handleClientDisconnect(client);
	
	// Send quit message to client, then close connection
	_sendMsg(client.getFd(), ":ft_irc QUIT :" + quitMsg + "\r\n");
	close(client.getFd());
}

// Handle PING command: Echo back PONG
void Server::_cmdPing(Client &client, const std::vector<std::string> &params)
{
	std::string response = ":ft_irc PONG ft_irc";
	if (!params.empty())
		response += " " + params[0];
	response += "\r\n";
	_sendMsg(client.getFd(), response);
}
