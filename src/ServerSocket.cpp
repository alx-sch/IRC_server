#include <cerrno>		// errno
#include <cstring>		// strerror()
#include <stdexcept>	// std::runtime_error()
#include <unistd.h>		// close()

#include <sys/socket.h>	// socket(), bind(), listen(), accept(), setsockopt(), etc.
#include <netinet/in.h>	// sockaddr_in, INADDR_ANY, htons()

#include "../include/Server.hpp"
#include "../include/utils.hpp"	// toString()

/**
 Used in the constructor.
 Initializes the server socket, the server-side endpoint for communication.

 It creates the socket using:
 - domain:		`AF_INET` for IPv4 addresses
 - type:		`SOCK_STREAM | SOCK_NONBLOCK` (TCP + non-blocking)
 - protocol:	`0` (default for `SOCK_STREAM` is TCP)

 It sets socket options to allow address reuse (`SO_REUSEADDR`), binds the
 socket to the specified port, and starts listening.
*/
void	Server::initSocket()
{
	// Create socket (IPv4, TCP, non-blocking)
	// Note: SOCK_NONBLOCK is not available on macOS — use fcntl() instead.
	_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_fd == -1)
		throw std::runtime_error("Failed to create server socket: " + std::string(strerror(errno)));

	// Avoid "Address already in use" error when restarting server quickly
	int	yes = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
	{
		close(_fd); // closing necessary here, as desctructor would not be called (error happens within constructor)
		throw std::runtime_error("Failed to set SO_REUSEADDR in server socket: " + std::string(strerror(errno)));
	}

	// Set up address struct for binding
	sockaddr_in	addr;
	memset(&addr, 0, sizeof(addr));		// Set all fields to zero
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);		// Makes sure port is in network byte order (big-endian)
	addr.sin_addr.s_addr = INADDR_ANY;	// Any address for socket binding (0.0.0.0)

	// Bind the socket to the address and port
	if (bind(_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
	{
		close(_fd);
		throw std::runtime_error("Failed to bind server socket to port: " + toString(_port) + ": " + strerror(errno));
	}

	// Start listening for incoming connections (max connections as allowed by system)
	if (listen(_fd, SOMAXCONN) == -1)
	{
		close(_fd);
		throw std::runtime_error("Failed to listen on server socket: " + std::string(strerror(errno)));
	}
}

/**
 Prepares the read fd_set for use with select().

 @param readFds 	Reference to the fd_set to be passed to select().
 @return			The highest file descriptor value among all monitored fds.
*/
int	Server::prepareReadSet(fd_set& readFds)
{
	FD_ZERO(&readFds);		// Clear the set before each select call
	FD_SET(_fd, &readFds);	// Add the listening socket fd to the read set
	int maxFd = _fd;

	// Add all active user sockets to readFds for monitoring
	for (std::map<int, User*>::iterator it = _usersFd.begin(); it != _usersFd.end(); ++it)
	{
		FD_SET(it->first, &readFds);
		if (it->first > maxFd) // Update maxFd if this user fd is larger
			maxFd = it->first;
	}

	return maxFd;
}
