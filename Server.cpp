#include "Server.hpp"

#include <iostream>

Server::Server(int port, const std::string &password)
:	_port(port),
	_password(password),
	_serverFd(-1)
{
	std::cout << "Server constructor called" << std::endl;
}

Server::~Server()
{
	std::cout << "Server destructor called" << std::endl;
}

void Server::run()
{
	std::cout << "Server::run() called" << std::endl;
}