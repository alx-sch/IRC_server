#include <iostream>

#include "../include/Server.hpp"
#include "../include/defines.hpp"	// for color definitions
#include "../include/signal.hpp"	// for setupSignalHandler()
#include "../include/utils.hpp"		// for parsePort()

int	main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr << YELLOW << "Usage: " << argv[0] << " <port> <password>\n" << RESET;
		return 1;
	}

	try
	{
		int		port = parsePort(argv[1]);	// Parse and validate port number
		Server	server(port, argv[2]);		// Create server instance with port and password

		setupSignalHandler();				// Set up signal handler for graceful shutdown via SIGINT
		server.run();						// Start the server loop
	}
	catch (const std::exception& e)
	{
		std::cerr << RED << BOLD << "Error: " << e.what() << "\n" << RESET;
		return 1; // Exit on error
	}

	return 0;
}
