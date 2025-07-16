#include <iostream>
#include <cstdlib>

#include "../include/defines.hpp"
#include "../include/Server.hpp"
#include "../include/signal.hpp"
#include "../include/utils.hpp"

int	main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr	<< YELLOW << "Usage: " << argv[0] << " <port> <password>\n" << RESET;
		return 1;
	}

	std::signal(SIGINT, handleSignal); // Handle Ctrl+C to gracefully shut down the server

	try
	{
		int port = parsePort(argv[1]); // Parse and validate port number
		Server server(port, argv[2]); // Create server instance with port and password
		server.start(); // Start the server loop
	}
	catch (const std::exception& e)
	{
		std::cerr << RED << BOLD << "Error: " << e.what() << "\n" << RESET;
		return 1; // Exit on error
	}

	return 0;
}
