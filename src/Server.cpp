#include <cerrno>		// errno
#include <cstring>		// memset(), strerror()
#include <iostream>	
#include <stdexcept>	// std::runtime_error()
#include <string>
#include <unistd.h>		// close()

#include <sys/select.h> // for select(), fd_set, FD_* macros

#include "../include/Server.hpp"
#include "../include/defines.hpp"	// color formatting
#include "../include/signal.hpp"	// g_running variable

Server::Server(int port, const std::string& password) 
	: _port(port), _fd(-1), _password(password)
{
	initSocket();
}

Server::~Server()
{
	// Close the listening socket if open
	if (_fd != -1)
		close(_fd);

	// Delete all dynamically allocated User objects
	while (!_usersFd.empty())
		deleteUser(_usersFd.begin()->first);
}

/**
 Starts the server loop and accepts incoming client connections.

 Enters an infinite `accept` loop, calling `accept()` on the listening socket.
 For each successful client connection, it prints the client's IP address and port,
 then immediately closes the connection (for testing purposes).

 Loop and function ends when SIGINT (Ctrl+C) is received, setting `g_running` to 0.
*/
void	Server::run()
{
	fd_set		readFds;	// struct to keep track of fds that are ready to read (connections, messages)
	int			maxFd;		// Highest-numbered fd in the read set
	int			ready;		// Number of ready fds

	std::cout << "Server running on port " << YELLOW << _port << RESET << std::endl;

	while (g_running)
	{
		maxFd = prepareReadSet(readFds);

		// Pause the program until a socket becomes readable (messages or new connections)
		ready = select(maxFd + 1, &readFds, NULL, NULL, NULL); // returns number of ready fds
		if (ready == -1)
		{
			if (errno == EINTR) // If interrupted by signal (SIGINT), just return to main.
				return;
			throw std::runtime_error("select() failed: " + std::string(strerror(errno)));
		}

		// Check if the server socket (_fd) is readable -> listening socket has a new connection
		if (FD_ISSET(_fd, &readFds))
			acceptNewUser();
	}
}
