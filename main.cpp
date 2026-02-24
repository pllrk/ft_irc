#include <string>
#include <iostream>
#include <cstdlib>
#include "Server.hpp"


int main(int argc, char** argv)
{
	int port;
	std::string password;

	if (argc != 3)
	{
		std::cerr << "Error :" << argv[0] << " need <port> <password>." << std::endl;
		return 1;
	}
	try
	{
		port = std::atoi(argv[1]);
		password = argv[2];
		if (port < 1024 || port > 65535)
			throw std::invalid_argument("Error: Port number must be between 1024 and 65535.");
		if (password.empty())
			throw std::invalid_argument("Error: Password cannot be empty.");
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	try
	{
		// Start server lifecycle: socket setup + poll loop.
		Server server(port, password);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}