#include <iostream>
#include <string>

#include "../include/Server.hpp"
#include "../include/defines.hpp"	// for color definitions
#include "../include/signal.hpp"	// for setupSignalHandler()
#include "../include/utils.hpp"		// for parsePort()

/**
 Entry point for the IRC server.

 Expects two command-line arguments:
 - port: 		The port number to listen on (1–65535)
 - password: 	The server password required for clients to connect

 Sets up signal handling, initializes the server, and starts the main loop.
*/
int	main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr	<< YELLOW << "Usage: " << argv[0] << " <port> <password>\n"
					<< "Example: " << argv[0] << " 6667 pw123" << RESET << std::endl;
		return 1;
	}

	try
	{
		int			port = parsePort(argv[1]);	// Parse and validate port number
		std::string	password = argv[2];
		Server		server(port, password);

		setupSignalHandler();	// Set up signal handler for graceful shutdown via SIGINT
		server.run();			// Start the server loop, only interrupted by SIGINT or throwing exceptions
	}
	catch (const std::exception& e)
	{
		std::cerr << RED << BOLD << "ERROR: " << e.what() << "\n" << RESET;
		return 1; // Exit on error
	}

	return 0;
}
