#include "Channel.hpp"

// Constructor: Initialize channel with name
Channel::Channel(const std::string &name)
	: _name(name), _topic("")
{
}

// Destructor: Clean up (members are not deleted as they're owned by Server)
Channel::~Channel()
{
	_members.clear();
	_operators.clear();
}

// Get channel name
std::string Channel::getName() const
{
	return _name;
}

// Get channel topic
std::string Channel::getTopic() const
{
	return _topic;
}

// Get all members
const std::map<int, Client*> &Channel::getMembers() const
{
	return _members;
}

// Check if client is channel operator
bool Channel::isOperator(int fd) const
{
	return _operators.find(fd) != _operators.end();
}

// Check if channel has member with given fd
bool Channel::hasMember(int fd) const
{
	return _members.find(fd) != _members.end();
}

// Set channel topic
void Channel::setTopic(const std::string &topic)
{
	_topic = topic;
}

// Add member to channel (first to join becomes operator)
void Channel::addMember(int fd, Client *client, bool isOp)
{
	_members[fd] = client;
	if (isOp || _members.size() == 1)
		_operators.insert(fd);
}

// Remove member from channel
void Channel::removeMember(int fd)
{
	_members.erase(fd);
	_operators.erase(fd);
}

// Promote client to operator
void Channel::promoteToOperator(int fd)
{
	if (_members.find(fd) != _members.end())
		_operators.insert(fd);
}

// Demote client from operator
void Channel::demoteFromOperator(int fd)
{
	_operators.erase(fd);
}

// Get member count
size_t Channel::getMemberCount() const
{
	return _members.size();
}
