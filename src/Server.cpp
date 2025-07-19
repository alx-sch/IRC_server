#include <iostream>
#include <string>
#include <stdexcept>	// std::runtime_error
#include <cerrno>		// errno
#include <cstring>		// memset(), strerror()

#include <unistd.h>		// close()
#include <sys/select.h>	// select(), fd_set, FD_* macros

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
 Starts the main server loop to handle incoming connections and client messages.

 Sets up the `fd_set` for `select()`, and continuously monitors:
 - The listening socket for new client connections.
 - All active user sockets for incoming messages.

 The loop runs until interrupted by `SIGINT` (Ctrl+C), at which point `g_running` becomes 0.
*/
void	Server::run()
{
	fd_set	readFds;	// Set of fds to monitor for readability
	int		maxFd;		// Highest fd in the set
	int		ready;		// Number of ready fds returned by select()

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

		// New incoming connection?
		if (FD_ISSET(_fd, &readFds))
			acceptNewUser(); // Adds user to `_usersFd`

		// Handle user input for all active connections (messages, disconnections)
		handleReadyUsers(readFds);
	}
}

std::string	Server::getPassword() const
{
	return _password;
}
