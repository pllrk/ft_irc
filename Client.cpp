#include "Client.hpp"
#include <iostream>

// Constructor: Initialize client with file descriptor and optional data
Client::Client(int fd, const std::string &nickname, const std::string &username, const std::string &realname)
	: _fd(fd), _nickname(nickname), _username(username), _realname(realname), _isRegistered(false), _isAuthenticated(false), _buffer("")
{
}

// Destructor: Clean up client
Client::~Client()
{
}

// Getters
int Client::getFd() const
{
	return _fd;
}

std::string Client::getNickname() const
{
	return _nickname;
}

std::string Client::getUsername() const
{
	return _username;
}

std::string Client::getRealname() const
{
	return _realname;
}

bool Client::isRegistered() const
{
	return _isRegistered;
}

bool Client::isAuthenticated() const
{
	return _isAuthenticated;
}

std::string Client::getBuffer() const
{
	return _buffer;
}

// Setters
void Client::setNickname(const std::string &nickname)
{
	_nickname = nickname;
}

void Client::setUsername(const std::string &username)
{
	_username = username;
}

void Client::setRealname(const std::string &realname)
{
	_realname = realname;
}

void Client::setRegistered(bool registered)
{
	_isRegistered = registered;
}

void Client::setAuthenticated(bool authenticated)
{
	_isAuthenticated = authenticated;
}

// Append data to the client's message buffer
void Client::appendToBuffer(const std::string &data)
{
	_buffer += data;
}

// Clear the client's message buffer
void Client::clearBuffer()
{
	_buffer.clear();
}
