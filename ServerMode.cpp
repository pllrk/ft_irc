#include "Server.hpp"
#include <cstdlib>

// MODE: Change channel modes (+i, +t, +k, +o, +l)
void Server::_cmdMode(Client &client, const std::vector<std::string> &params)
{
	if (params.empty())
		return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");

	const std::string &channelName = params[0];
	if (channelName.empty() || channelName[0] != '#')
		return;
	if (!client.isRegistered())
		return _sendMsg(client.getFd(), ":ircserv 451 * :You have not registered\r\n");

	std::map<std::string, Channel*>::iterator chanIt = _channels.find(channelName);
	if (chanIt == _channels.end())
		return _sendMsg(client.getFd(), ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");

	Channel *channel = chanIt->second;

	// Query current modes
	if (params.size() < 2)
	{
		std::string modeStr = "+";
		if (channel->isInviteOnly())		modeStr += "i"; // i: invite-only channel
		if (channel->isTopicRestricted())	modeStr += "t"; // t: only operators can set topic
		if (!channel->getKey().empty())		modeStr += "k"; // k: channel has a key/password
		if (channel->getUserLimit() > 0)	modeStr += "l"; // l: channel has a user limit
		return _sendMsg(client.getFd(), ":ircserv 324 " + client.getNickname() + " " + channelName + " " + modeStr + "\r\n");
	}

	if (!channel->isOperator(client.getFd()))
		return _sendMsg(client.getFd(), ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");

	const std::string &modeStr = params[1];
	size_t argIdx = 2;
	bool adding = true;
	std::string appliedModes;
	std::string appliedArgs;

	for (size_t i = 0; i < modeStr.size(); ++i)
	{
		char m = modeStr[i];
		if (m == '+') { adding = true;  continue; }
		if (m == '-') { adding = false; continue; }

		if (m == 'i')
		{
			channel->setInviteOnly(adding);
			appliedModes += (adding ? "+i" : "-i");
		}
		else if (m == 't')
		{
			channel->setTopicRestricted(adding);
			appliedModes += (adding ? "+t" : "-t");
		}
		else if (m == 'k')
		{
			if (adding)
			{
				if (argIdx >= params.size())
					return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
				channel->setKey(params[argIdx]);
				appliedArgs += " " + params[argIdx];
				++argIdx;
			}
			else
				channel->setKey("");
			appliedModes += (adding ? "+k" : "-k");
		}
		else if (m == 'o')
		{
			if (argIdx >= params.size())
				return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
			const std::string &targetNick = params[argIdx++];
			std::map<std::string, int>::iterator nickIt = _nickToFd.find(targetNick);
			if (nickIt == _nickToFd.end() || !channel->hasMember(nickIt->second))
				return _sendMsg(client.getFd(), ":ircserv 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
			if (adding)
				channel->promoteToOperator(nickIt->second);
			else
				channel->demoteFromOperator(nickIt->second);
			appliedModes += (adding ? "+o" : "-o");
			appliedArgs += " " + targetNick;
		}
		else if (m == 'l')
		{
			if (adding)
			{
				if (argIdx >= params.size())
					return _sendMsg(client.getFd(), ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
				int limit = std::atoi(params[argIdx].c_str());
				if (limit <= 0)
					return _sendMsg(client.getFd(), ":ircserv 696 " + client.getNickname() + " " + channelName + " l * :Invalid mode parameter\r\n");
				channel->setUserLimit(limit);
				appliedArgs += " " + params[argIdx];
				++argIdx;
			}
			else
				channel->setUserLimit(0);
			appliedModes += (adding ? "+l" : "-l");
		}
		else
			continue;
	}

	if (!appliedModes.empty())
	{
		std::string modeChangeMsg = ":" + client.getNickname() + " MODE " + channelName + " " + appliedModes + appliedArgs + "\r\n";
		_broadcastToChannel(channel, modeChangeMsg, client.getFd());
		_sendMsg(client.getFd(), modeChangeMsg);
	}
}
