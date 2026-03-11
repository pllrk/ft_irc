#include "Server.hpp"

// Helper: broadcast a message to all members of a channel
void Server::_broadcastToChannel(Channel *channel, const std::string &msg, int exceptFd)
{
	const std::map<int, Client*> &members = channel->getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
		if (it->first != exceptFd)
			_sendMsg(it->first, msg);
}

// Handle JOIN command: Add client to channel
void Server::_cmdJoin(Client &client, const std::vector<std::string> &params)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 464 * :You must be registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ircserv 461 * JOIN :Not enough parameters\r\n");

	std::string channelName = params[0];
	std::string key = (params.size() > 1) ? params[1] : "";

	// Channel names must start with #
	if (channelName.empty() || channelName[0] != '#')
		return _sendMsg(client.getFd(), ":ircserv 403 * " + channelName + " :No such channel\r\n");

	// Create channel if it doesn't exist
	bool isNew = (_channels.find(channelName) == _channels.end());
	if (isNew)
		_channels[channelName] = new Channel(channelName);

	Channel *channel = _channels[channelName];

	if (channel->hasMember(client.getFd()))
		return;

	// Check invite-only
	if (!isNew && channel->isInviteOnly() && !channel->isInvited(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");

	// Check key
	if (!isNew && !channel->getKey().empty() && channel->getKey() != key)
		return _sendMsg(client.getFd(), ":ircserv 475 " + client.getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");

	// Check user limit
	if (!isNew && channel->getUserLimit() > 0 && (int)channel->getMemberCount() >= channel->getUserLimit())
		return _sendMsg(client.getFd(), ":ircserv 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");

	// Add client to channel
	channel->addMember(client.getFd(), &client, isNew);
	std::string prefix = ":" + _clientPrefix(client);

	// Send JOIN confirmation to client
	_sendMsg(client.getFd(), prefix + " JOIN " + channelName + "\r\n");

	// Broadcast to all members in channel
	const std::map<int, Client*> &members = channel->getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin();
		 it != members.end(); ++it)
	{
		if (it->first != client.getFd())
			_sendMsg(it->first, prefix + " JOIN " + channelName + "\r\n");
	}

	// Send topic if set
	if (!channel->getTopic().empty())
		_sendMsg(client.getFd(), ":ircserv 332 " + client.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");

	// Send NAMES list to client
	std::string namesList = ":ircserv 353 " + client.getNickname() + " = " + channelName + " :";
	for (std::map<int, Client*>::const_iterator it = members.begin();
		 it != members.end(); ++it)
	{
		if (it != members.begin())
			namesList += " ";
		if (channel->isOperator(it->first))
			namesList += "@";
		namesList += it->second->getNickname();
	}
	namesList += "\r\n";
	_sendMsg(client.getFd(), namesList);

	// Send end of NAMES
	_sendMsg(client.getFd(), ":ircserv 366 " + client.getNickname() + " " + channelName + " :End of /NAMES list\r\n");
}

// Handle PART command: Remove client from channel
void Server::_cmdPart(Client &client, const std::vector<std::string> &params)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 464 * :You must be registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ircserv 461 * PART :Not enough parameters\r\n");

	std::string channelName = params[0];

	// Check if channel exists
	std::map<std::string, Channel*>::iterator chanIt = _channels.find(channelName);
	if (chanIt == _channels.end())
		return _sendMsg(client.getFd(), ":ircserv 403 * " + channelName + " :No such channel\r\n");

	Channel *channel = chanIt->second;

	// Check if client is in channel
	if (!channel->hasMember(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 441 * " + channelName + " :You're not on that channel\r\n");

	// Broadcast PART to all members
	std::string partMsg = ":" + _clientPrefix(client) + " PART " + channelName;
	if (params.size() > 1 && !params[1].empty())
		partMsg += " :" + params[1];
	partMsg += "\r\n";

	const std::map<int, Client*> &members = channel->getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin();
		 it != members.end(); ++it)
	{
		_sendMsg(it->first, partMsg);
	}

	// Remove client from channel
	channel->removeMember(client.getFd());

	// Delete channel if empty
	if (channel->getMemberCount() == 0)
	{
		delete channel;
		_channels.erase(chanIt);
	}
}

// Handle PRIVMSG command: Send message to user or channel
void Server::_cmdPrivMsg(Client &client, const std::vector<std::string> &params, const std::string &msg)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 464 * :You must be registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ircserv 411 * :No recipient given (PRIVMSG)\r\n");

	std::string target = params[0];
	size_t msgPos = msg.find(':');
	if (msgPos == std::string::npos)
		return _sendMsg(client.getFd(), ":ircserv 412 * :No text to send\r\n");

	std::string message = msg.substr(msgPos + 1);

	// Check if target is a channel
	if (target[0] == '#')
	{
		std::map<std::string, Channel*>::iterator chanIt = _channels.find(target);
		if (chanIt == _channels.end())
			return _sendMsg(client.getFd(), ":ircserv 403 * " + target + " :No such channel\r\n");

		Channel *channel = chanIt->second;
		if (!channel->hasMember(client.getFd()))
			return _sendMsg(client.getFd(), ":ircserv 404 * " + target + " :Cannot send to channel\r\n");

		// Broadcast to all channel members except sender
		std::string fullMsg = ":" + _clientPrefix(client) + " PRIVMSG " + target + " :" + message + "\r\n";
		const std::map<int, Client*> &members = channel->getMembers();
		for (std::map<int, Client*>::const_iterator it = members.begin();
			 it != members.end(); ++it)
		{
			if (it->first != client.getFd())
				_sendMsg(it->first, fullMsg);
		}
	}
	else
	{
		// Direct message to user
		std::map<std::string, int>::iterator nickIt = _nickToFd.find(target);
		if (nickIt == _nickToFd.end())
			return _sendMsg(client.getFd(), ":ircserv 401 * " + target + " :No such nick\r\n");

		std::string fullMsg = ":" + _clientPrefix(client) + " PRIVMSG " + target + " :" + message + "\r\n";
		_sendMsg(nickIt->second, fullMsg);
	}
}

void Server::_handleClientDisconnect(Client &client)
{
	std::string quitMsg = ":" + _clientPrefix(client) + " QUIT :" + client.getQuitMessage() + "\r\n";

	std::map<std::string, Channel*>::iterator it = _channels.begin();
	while (it != _channels.end())
	{
		Channel *channel = it->second;
		if (channel->hasMember(client.getFd()))
		{
			const std::map<int, Client*> &members = channel->getMembers();
			for (std::map<int, Client*>::const_iterator mit = members.begin();
				 mit != members.end(); ++mit)
			{
				if (mit->first != client.getFd())
					_sendMsg(mit->first, quitMsg);
			}
			channel->removeMember(client.getFd());
			if (channel->getMemberCount() == 0)
			{
				delete channel;
				_channels.erase(it++);
				continue;
			}
		}
		++it;
	}
}

// KICK: Eject a client from a channel
void Server::_cmdKick(Client &client, const std::vector<std::string> &params)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 451 * :You have not registered\r\n");
	if (params.size() < 2)
		return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " KICK :Not enough parameters\r\n");

	const std::string &channelName = params[0];
	const std::string &targetNick  = params[1];
	std::string reason = (params.size() > 2) ? params[2] : client.getNickname();

	std::map<std::string, Channel*>::iterator chanIt = _channels.find(channelName);
	if (chanIt == _channels.end())
		return _sendMsg(client.getFd(), ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");

	Channel *channel = chanIt->second;

	if (!channel->hasMember(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n");
	if (!channel->isOperator(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");

	std::map<std::string, int>::iterator nickIt = _nickToFd.find(targetNick);
	if (nickIt == _nickToFd.end() || !channel->hasMember(nickIt->second))
		return _sendMsg(client.getFd(), ":ircserv 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");

	int targetFd = nickIt->second;
	std::string kickMsg = ":" + _clientPrefix(client) + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
	_broadcastToChannel(channel, kickMsg, -1);
	channel->removeMember(targetFd);

	if (channel->getMemberCount() == 0)
	{
		delete channel;
		_channels.erase(chanIt);
	}
}

// INVITE: Invite a client to a channel
void Server::_cmdInvite(Client &client, const std::vector<std::string> &params)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 451 * :You have not registered\r\n");
	if (params.size() < 2)
		return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " INVITE :Not enough parameters\r\n");

	const std::string &targetNick  = params[0];
	const std::string &channelName = params[1];

	std::map<std::string, Channel*>::iterator chanIt = _channels.find(channelName);
	if (chanIt == _channels.end())
		return _sendMsg(client.getFd(), ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");

	Channel *channel = chanIt->second;

	if (!channel->hasMember(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n");
	if (channel->isInviteOnly() && !channel->isOperator(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");

	std::map<std::string, int>::iterator nickIt = _nickToFd.find(targetNick);
	if (nickIt == _nickToFd.end())
		return _sendMsg(client.getFd(), ":ircserv 401 " + client.getNickname() + " " + targetNick + " :No such nick\r\n");

	int targetFd = nickIt->second;
	if (channel->hasMember(targetFd))
		return _sendMsg(client.getFd(), ":ircserv 443 " + client.getNickname() + " " + targetNick + " " + channelName + " :is already on channel\r\n");

	channel->addInvite(targetFd);
	_sendMsg(client.getFd(), ":ircserv 341 " + client.getNickname() + " " + targetNick + " " + channelName + "\r\n");
	_sendMsg(targetFd, ":" + _clientPrefix(client) + " INVITE " + targetNick + " " + channelName + "\r\n");
}

// TOPIC: View or change channel topic
void Server::_cmdTopic(Client &client, const std::vector<std::string> &params, const std::string &msg)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 451 * :You have not registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " TOPIC :Not enough parameters\r\n");

	const std::string &channelName = params[0];

	std::map<std::string, Channel*>::iterator chanIt = _channels.find(channelName);
	if (chanIt == _channels.end())
		return _sendMsg(client.getFd(), ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");

	Channel *channel = chanIt->second;

	if (!channel->hasMember(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n");

	// Query topic
	size_t colonPos = msg.find(':');
	if (params.size() < 2 && colonPos == std::string::npos)
	{
		if (channel->getTopic().empty())
			return _sendMsg(client.getFd(), ":ircserv 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n");
		return _sendMsg(client.getFd(), ":ircserv 332 " + client.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
	}

	// Set topic
	if (channel->isTopicRestricted() && !channel->isOperator(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");

	std::string newTopic = (colonPos != std::string::npos) ? msg.substr(colonPos + 1) : params[1];
	channel->setTopic(newTopic);

	std::string topicMsg = ":" + _clientPrefix(client) + " TOPIC " + channelName + " :" + newTopic + "\r\n";
	_broadcastToChannel(channel, topicMsg, -1);
}

void Server::_cmdWho(Client &client, const std::vector<std::string> &params)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 451 * :You have not registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " WHO :Not enough parameters\r\n");

	const std::string &target = params[0];
	if (target.empty() || target[0] != '#')
	{
		_sendMsg(client.getFd(), ":ircserv 315 " + client.getNickname() + " " + target + " :End of /WHO list\r\n");
		return;
	}

	std::map<std::string, Channel*>::iterator chanIt = _channels.find(target);
	if (chanIt == _channels.end())
	{
		_sendMsg(client.getFd(), ":ircserv 315 " + client.getNickname() + " " + target + " :End of /WHO list\r\n");
		return;
	}

	Channel *channel = chanIt->second;
	const std::map<int, Client*> &members = channel->getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		Client *member = it->second;
		std::string flags = "H";
		if (channel->isOperator(it->first))
			flags += "@";
		_sendMsg(client.getFd(), ":ircserv 352 " + client.getNickname() + " " + target + " "
			+ member->getUsername() + " localhost ircserv " + member->getNickname() + " "
			+ flags + " :0 " + member->getRealname() + "\r\n");
	}
	_sendMsg(client.getFd(), ":ircserv 315 " + client.getNickname() + " " + target + " :End of /WHO list\r\n");
}

