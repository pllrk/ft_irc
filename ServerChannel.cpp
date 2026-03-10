#include "Server.hpp"

// Handle JOIN command: Add client to channel
void Server::_cmdJoin(Client &client, const std::vector<std::string> &params)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ft_irc 464 * :You must be registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ft_irc 461 * JOIN :Not enough parameters\r\n");

	std::string channelName = params[0];

	// Channel names must start with #
	if (channelName.empty() || channelName[0] != '#')
		return _sendMsg(client.getFd(), ":ft_irc 403 * " + channelName + " :No such channel\r\n");

	// Create channel if it doesn't exist
	if (_channels.find(channelName) == _channels.end())
		_channels[channelName] = new Channel(channelName);

	Channel *channel = _channels[channelName];

	// Add client to channel
	if (!channel->hasMember(client.getFd()))
		channel->addMember(client.getFd(), &client, channel->getMemberCount() == 0);

	// Send JOIN confirmation to client
	_sendMsg(client.getFd(), ":" + client.getNickname() + " JOIN " + channelName + "\r\n");

	// Broadcast to all members in channel
	const std::map<int, Client*> &members = channel->getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin();
		 it != members.end(); ++it)
	{
		if (it->first != client.getFd())
			_sendMsg(it->first, ":" + client.getNickname() + " JOIN " + channelName + "\r\n");
	}

	// Send NAMES list to client
	std::string namesList = ":ft_irc 353 " + client.getNickname() + " = " + channelName + " :";
	for (std::map<int, Client*>::const_iterator it = members.begin();
		 it != members.end(); ++it)
	{
		if (namesList.length() > 1)
			namesList += " ";
		if (channel->isOperator(it->first))
			namesList += "@";
		namesList += it->second->getNickname();
	}
	namesList += "\r\n";
	_sendMsg(client.getFd(), namesList);

	// Send end of NAMES
	_sendMsg(client.getFd(), ":ft_irc 366 " + client.getNickname() + " " + channelName + " :End of /NAMES list\r\n");
}

// Handle PART command: Remove client from channel
void Server::_cmdPart(Client &client, const std::vector<std::string> &params)
{
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ft_irc 464 * :You must be registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ft_irc 461 * PART :Not enough parameters\r\n");

	std::string channelName = params[0];

	// Check if channel exists
	std::map<std::string, Channel*>::iterator chanIt = _channels.find(channelName);
	if (chanIt == _channels.end())
		return _sendMsg(client.getFd(), ":ft_irc 403 * " + channelName + " :No such channel\r\n");

	Channel *channel = chanIt->second;

	// Check if client is in channel
	if (!channel->hasMember(client.getFd()))
		return _sendMsg(client.getFd(), ":ft_irc 441 * " + channelName + " :You're not on that channel\r\n");

	// Broadcast PART to all members
	std::string partMsg = ":" + client.getNickname() + " PART " + channelName;
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
		return _sendMsg(client.getFd(), ":ft_irc 464 * :You must be registered\r\n");
	if (params.empty())
		return _sendMsg(client.getFd(), ":ft_irc 411 * :No recipient given (PRIVMSG)\r\n");

	std::string target = params[0];
	size_t msgPos = msg.find(':');
	if (msgPos == std::string::npos)
		return _sendMsg(client.getFd(), ":ft_irc 412 * :No text to send\r\n");

	std::string message = msg.substr(msgPos + 1);

	// Check if target is a channel
	if (target[0] == '#')
	{
		std::map<std::string, Channel*>::iterator chanIt = _channels.find(target);
		if (chanIt == _channels.end())
			return _sendMsg(client.getFd(), ":ft_irc 403 * " + target + " :No such channel\r\n");

		Channel *channel = chanIt->second;
		if (!channel->hasMember(client.getFd()))
			return _sendMsg(client.getFd(), ":ft_irc 404 * " + target + " :Cannot send to channel\r\n");

		// Broadcast to all channel members except sender
		std::string fullMsg = ":" + client.getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";
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
			return _sendMsg(client.getFd(), ":ft_irc 401 * " + target + " :No such nick\r\n");

		std::string fullMsg = ":" + client.getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";
		_sendMsg(nickIt->second, fullMsg);
	}
}

void Server::_handleClientDisconnect(Client &client)
{
    std::string quitMsg = ":" + client.getNickname() + " QUIT :Client disconnected\r\n";
    
    // Iterate through all channels
    std::map<std::string, Channel*>::iterator it = _channels.begin();
    while (it != _channels.end())
    {
        Channel *channel = it->second;
        
        // If client is in this channel
        if (channel->hasMember(client.getFd()))
        {
            // Notify all other members
            const std::map<int, Client*> &members = channel->getMembers();
            for (std::map<int, Client*>::const_iterator mit = members.begin();
                 mit != members.end(); ++mit)
            {
                if (mit->first != client.getFd())
                    _sendMsg(mit->first, quitMsg);
            }
            
            // Remove client from channel
            channel->removeMember(client.getFd());
            
            // Delete channel if empty
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
