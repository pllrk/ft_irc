#include "Server.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <vector>
#include <poll.h>
#include <cstdlib>
#include <map>

// Constructor: Initialize server with port and password
Server::Server(int port, const std::string &password)
	: _port(port),
	  _serverFd(-1),
	  _password(password)
{
}

// Destructor: Clean up all clients and close server socket
Server::~Server()
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	_clients.clear();
	_nickToFd.clear();

	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	_channels.clear();

	if (_serverFd != -1)
		close(_serverFd);
}

// Main entry point: Set up server socket and start the event loop
void Server::run()
{
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd == -1)
	{
		std::cerr << "Error: Failed to create socket" << std::endl;
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::cerr << "Error: setsockopt failed" << std::endl;
		close(_serverFd);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(_port);

	if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		std::cerr << "Error: bind failed" << std::endl;
		close(_serverFd);
		exit(EXIT_FAILURE);
	}

	if (listen(_serverFd, 5) == -1)
	{
		std::cerr << "Error: listen failed" << std::endl;
		close(_serverFd);
		exit(EXIT_FAILURE);
	}

	std::cout << "Server listening on port " << _port << std::endl;
	_eventloop();
}

// Main poll loop: Accept connections and handle client data
void Server::_eventloop()
{
	std::vector<struct pollfd> pollfds;
	struct pollfd listenPollfd;

	listenPollfd.fd = _serverFd;
	listenPollfd.events = POLLIN;
	listenPollfd.revents = 0;
	pollfds.push_back(listenPollfd);

	while (true)
	{
		int pollCount = poll(pollfds.data(), pollfds.size(), -1);
		if (pollCount == -1)
		{
			std::cerr << "Error: poll failed" << std::endl;
			break;
		}

		if (pollfds[0].revents & POLLIN)
		{
			struct sockaddr_in clientAddr;
			socklen_t clientAddrLen = sizeof(clientAddr);
			int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);

			if (clientFd == -1)
			{
				std::cerr << "Error: accept failed" << std::endl;
			}
			else
			{
				Client *c = new Client(clientFd, "", "", "");
				_clients[clientFd] = c;

				struct pollfd clientPollfd;
				clientPollfd.fd = clientFd;
				clientPollfd.events = POLLIN;
				clientPollfd.revents = 0;
				pollfds.push_back(clientPollfd);

				std::cout << "Client connected: fd " << clientFd << std::endl;
			}
		}
		_handleClientData(pollfds);
	}
}

// Handle incoming data from all connected clients
void Server::_handleClientData(std::vector<struct pollfd> &pollfds)
{
	for (size_t i = 1; i < pollfds.size(); ++i)
	{
		if (!(pollfds[i].revents & POLLIN))
			continue;

		char buffer[512];
		ssize_t bytesRead = recv(pollfds[i].fd, buffer, sizeof(buffer) - 1, 0);

		if (bytesRead <= 0)
		{
			Client *c = _clients[pollfds[i].fd];
			if (c)
			{
				// Notify other users in channels before deletion
				_handleClientDisconnect(*c);
				if (!c->getNickname().empty())
					_nickToFd.erase(c->getNickname());
				delete c;
			}
			_clients.erase(pollfds[i].fd);
			close(pollfds[i].fd);
			pollfds.erase(pollfds.begin() + i);
			--i;

			std::cout << "Client disconnected: fd " << (pollfds.size() > i ? pollfds[i].fd : -1) << std::endl;
		}
		else
		{
			buffer[bytesRead] = '\0';

			Client *c = _clients[pollfds[i].fd];
			if (!c)
				continue;

			c->appendToBuffer(std::string(buffer));

			std::string pending = c->getBuffer();
			c->clearBuffer();

			size_t pos = 0;
			while ((pos = pending.find('\n')) != std::string::npos)
			{
				std::string line = pending.substr(0, pos);
				pending.erase(0, pos + 1);

				if (!line.empty() && line[line.size() - 1] == '\r')
					line.erase(line.size() - 1);

				if (!line.empty())
					_handleCommand(*c, line);
			}

			if (!pending.empty())
				c->appendToBuffer(pending);
		}
	}
}
