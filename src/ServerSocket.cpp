#include <cerrno>		// errno
#include <cstring>		// strerror()
#include <stdexcept>	// std::runtime_error
#include <string>		// std::string

#include <unistd.h>		// close()
#include <sys/socket.h>	// socket(), bind(), listen(), accept(), setsockopt(), etc.
#include <netinet/in.h>	// sockaddr_in, INADDR_ANY, htons()

#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/utils.hpp"	// toString()

/////////////////
// Init Socket //
/////////////////

/**
 Initializes the server socket. Used in `Server` constructor.

 Called from the constructor. This method sets up the server socket by:
 1.	Creating a non-blocking TCP socket.
 2.	Setting socket options (SO_REUSEADDR).
 3.	Binding the socket to the configured port.
 4.	Listening for incoming connections.
*/
void	Server::initSocket()
{
	createSocket();
	setSocketOptions();
	bindSocket();
	startListening();
}

/**
 Creates a non-blocking TCP socket.

 Uses `AF_INET` (IPv4), `SOCK_STREAM` (TCP), and `SOCK_NONBLOCK` (non-blocking mode).
 Note: `SOCK_NONBLOCK` is not available on macOS — use `fcntl()` instead.
*/
void	Server::createSocket()
{
	_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);  // Create non-blocking IPv4 TCP socket
	if (_fd == -1)
		throw std::runtime_error("Failed to create server socket: " + std::string(strerror(errno)));
}

/**
 Sets socket options to allow address reuse.

 Enables `SO_REUSEADDR` to allow quick server restarts on the same port.
*/
void	Server::setSocketOptions()
{
	int	yes = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
	{
		close(_fd); // Need to close here as destructor won't be called if constructor fails
		throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
	}
}

// Binds the server socket to a local address and port.
void	Server::bindSocket()
{
	sockaddr_in	addr;	// struct to represent an IPv4 socket address
	memset(&addr, 0, sizeof(addr));		// Zero out the struct
	addr.sin_family = AF_INET;			// IPv4
	addr.sin_port = htons(_port);		// Converts 16-bit int port number from host byte order to network byte order
	addr.sin_addr.s_addr = INADDR_ANY;	// Bind to all available network interfaces (INADDR_ANY = 0.0.0.0) -> accepts connections from local machine, local network, public internet, etc.

	if (bind(_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
	{
		close(_fd); // Errors might be restricted ports or ports already in use
		throw std::runtime_error("Failed to bind to port " + toString(_port) + ": " + strerror(errno));
	}
}

// Starts listening for incoming connections.
void	Server::startListening()
{
	if (listen(_fd, SOMAXCONN) == -1)	// Use system's max pending connection queue size (SOMAXCONN)
	{
		close(_fd);
		throw std::runtime_error("Failed to listen on server socket: " + std::string(strerror(errno)));
	}
}

////////////////////
// Prepare fd_set //
////////////////////

/**
 Prepares the read fd_set for use with select().
 Read set includes:
 - Server listening socket: A new user wants to connect.
 - User sockets: Clients have sent messages waiting to be read.

 @param readFds 	Reference to the fd_set to be passed to select().
 @return			The highest file descriptor value among all monitored fds.
*/
int	Server::prepareReadSet(fd_set& readFds)
{
	FD_ZERO(&readFds);		// Clear the set before each 'select' call
	FD_SET(_fd, &readFds);	// Add the listening socket fd to the read set
	int maxFd = _fd;

	// Add all active user sockets to readFds for monitoring
	for (std::map<int, User*>::const_iterator it = _usersFd.begin(); it != _usersFd.end(); ++it)
	{
		FD_SET(it->first, &readFds);
		if (it->first > maxFd) // Update maxFd if this user fd is larger
			maxFd = it->first;
	}

	return maxFd;
}

/**
 Prepares the write fd_set for use with select().
 Only includes users that have data in their output buffer.

 Write set includes:
 - User sockets: Ready to accept outgoing data without blocking.

 @param writeFds 	Reference to the fd_set to be passed to select().
 @return			The highest file descriptor value among all monitored fds.
*/
int	Server::prepareWriteSet(fd_set& writeFds)
{
	FD_ZERO(&writeFds);		// Clear the set before each select call
	int maxFd = -1;

	// Add user sockets with pending output to writeFds for monitoring
	for (std::map<int, User*>::const_iterator it = _usersFd.begin(); it != _usersFd.end(); ++it)
	{
		User* user = it->second;
		if (user && !user->getOutputBuffer().empty())
		{
			FD_SET(it->first, &writeFds); // add user fd to write set if output buffer is not empty
			if (it->first > maxFd)
				maxFd = it->first;
		}
	}

	return maxFd;
}
