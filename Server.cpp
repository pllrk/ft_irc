#include "Server.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <vector>
#include <poll.h>
#include <cstdlib>

Server::Server(int port, const std::string &password)
:	_port(port),
	_serverFd(-1),
	_password(password)
{
	std::cout << "Server constructor called" << std::endl;
}

Server::~Server()
{
	if (_serverFd != -1)
		close(_serverFd);
	std::cout << "Server destructor called" << std::endl;
}

void Server::run()
{
	std::cout << "Server::run() called" << std::endl;
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd == -1)
	{
		std::cerr << "Failed to create socket" << std::endl;
		exit(EXIT_FAILURE);
	}
	int opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::cerr << "Failed to set socket options" << std::endl;
		close(_serverFd);
		exit(EXIT_FAILURE);
	}
	std::cout << "Socket created and options set successfully" << std::endl;
	
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces - to change if needed.
	addr.sin_port = htons(_port);
	if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		std::cerr << "Failed to bind socket" << std::endl;
		close(_serverFd);
		exit(EXIT_FAILURE);
	}
	std::cout << "Socket bound to port " << _port << " successfully" << std::endl;
	if (listen(_serverFd, 5) == -1)
	{
		std::cerr << "Failed to listen on socket" << std::endl;
		close(_serverFd);
		exit(EXIT_FAILURE);
	}
	std::cout << "Server is listening on port " << _port << std::endl;
	_eventloop();
}

void Server::_eventloop()
{
	std::cout << "Entering event loop" << std::endl;
	
	std::vector<struct pollfd> pollfds;
	struct pollfd listenPollfd;

	listenPollfd.fd = _serverFd;
	listenPollfd.events = POLLIN;
	listenPollfd.revents = 0;
	pollfds.push_back(listenPollfd);

	std::cout << "Entering main event loop" << std::endl;
	while (true)
	{
		int pollCount = poll(pollfds.data(), pollfds.size(), -1);
		if (pollCount == -1)
		{
			std::cerr << "Poll error" << std::endl;
			break;
		}
		if (pollfds[0].revents & POLLIN)
		{
			struct sockaddr_in clientAddr;
			socklen_t clientAddrLen = sizeof(clientAddr);
			int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);

			if (clientFd == -1)
				std::cerr << "Failed to accept new connection" << std::endl;
			else
			{
				std::cout << "New client connected: fd " << clientFd << std::endl;
				struct pollfd clientPollfd;
				clientPollfd.fd = clientFd;
				clientPollfd.events = POLLIN;
				clientPollfd.revents = 0;
				pollfds.push_back(clientPollfd);
			}
		}
		_handleClientData(pollfds);
	}
}

void Server::_handleClientData(std::vector<struct pollfd> &pollfds)
{
	for (size_t i = 1; i < pollfds.size(); ++i)
	{
		if (pollfds[i].revents & POLLIN)
		{
			char buffer[512];
			ssize_t bytesRead = recv(pollfds[i].fd, buffer, sizeof(buffer) - 1, 0);
			if (bytesRead <= 0)
			{
				// Client disconnected or error
				std::cout << "Client disconnected: fd " << pollfds[i].fd << std::endl;
				close(pollfds[i].fd);
				pollfds.erase(pollfds.begin() + i);
				--i; // Adjust index after erase
			}
			else
			{
				buffer[bytesRead] = '\0';
				std::cout << "Received from client fd " << pollfds[i].fd << ": " << buffer;
				// IRC protocol handling will go here later
			}
		}
	}
}