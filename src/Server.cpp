
// C++ standard headers
#include <iostream>	
#include <stdexcept>	// std::runtime_error()
#include <cstring>		// memset(), strerror()
#include <cerrno>		// errno
#include <unistd.h>		// close()

// Socket and networking headers
#include <arpa/inet.h>	// inet_ntoa(), htons(), ntohs()
#include <netinet/in.h>	// sockaddr_in, INADDR_ANY
#include <sys/socket.h>	// socket, bind, listen, accept, setsockopt, etc.

#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/defines.hpp"	// color formatting
#include "../include/signal.hpp"	// g_running variable
#include "../include/utils.hpp"		// toString()

// just testing
#include <unistd.h> // for usleep()

/////////////////////////////////
// Constructors and Destructor //
/////////////////////////////////

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
	for (std::map<int, User*>::iterator it = _users.begin(); it != _users.end(); ++it)
	{
		delete it->second; // second is a pointer to User
		it->second = NULL;
	}
}

/////////////
// Methods //
/////////////

/**
 Initializes the server socket, the server-side endpoint for communication.

 It creates the socket using:
 - domain:		`AF_INET` for IPv4 addresses
 - type:		`SOCK_STREAM | SOCK_NONBLOCK` (TCP + non-blocking)
 - protocol:	`0` (default for `SOCK_STREAM` is TCP)

  It sets socket options to allow address reuse (`SO_REUSEADDR`), binds the
  socket to the specified port, and starts listening.

 Ressources:
 - socket: https://man7.org/linux/man-pages/man2/socket.2.html
 - https://beej.us/guide/bgnet/html/
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
		close(_fd);
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
		throw std::runtime_error("Failed to bind server socket to port " + toString(_port) + ": " + strerror(errno));

	// Start listening for incoming connections (max connections as allowed by system)
	if (listen(_fd, SOMAXCONN) == -1)
		throw std::runtime_error("Failed to listen on server socket: " + std::string(strerror(errno)));
}

/**
 Starts the server loop and accepts incoming client connections.

 Enters an infinite loop, calling `accept()` on the listening socket.
 For each successful client connection, it prints the client's IP address and port,
 then immediately closes the connection (for testing purposes).

 Loop and function ends when SIGINT (Ctrl+C) is received, setting `g_running` to 0.
*/
void	Server::start()
{
	int			userFd;			// fd for the accepted user connection
	sockaddr_in	userAddr;		// Init user address structure
	socklen_t	userLen = sizeof(userAddr);

	std::cout << "Server running on port " << YELLOW << _port << RESET << std::endl;

	while (g_running)
	{
		// Accept incoming connections
		userFd = accept(_fd, reinterpret_cast<sockaddr*>(&userAddr), &userLen);
		if (userFd == -1) // Also returns -1 when no pending connections are queued
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				usleep(100000); // Sleep for 100ms to avoid busy-waiting --> removed when select() is implemented
				continue; // No pending connections — try again --> loop start over
			}
			throw std::runtime_error("Failed to accept connection: " + std::string(strerror(errno)));
		}

		std::cout	<< "User "<< YELLOW << "#" << userFd << RESET << " connected from "
					<< YELLOW << inet_ntoa(userAddr.sin_addr)	// IP address as string
					<< ":" << ntohs(userAddr.sin_port)			// Port (convert from network order)
					<< RESET << std::endl;

		// Store new user
		User*	newUser = new User(); // maybe get info for username , nickname and have parametrized constructor
		_users[userFd] = newUser;

		////////////////////
		// TESTING ONLY !!///
		////////////////////
		getUser(userFd)->setNickname("Guest" + toString(userFd)); // Set default nickname
		getUser(userFd)->setUsername("User" + toString(userFd)); // Set default username

		// print user info, just testing
		if (getUser(userFd) == NULL)
			std::cerr << RED << "Error: User not found for fd " << userFd << RESET << std::endl;
		else
		{
			std::cout	<< "New user info: "
						<< "Nickname: " << YELLOW << getUser(userFd)->getNickname() << RESET
						<< ", Username: " << YELLOW << getUser(userFd)->getUsername() << RESET
						<< std::endl;
		}

		userFd += 1; // Increment fd to simulate invalid access (for testing purposes)
		if (getUser(userFd) == NULL)
			std::cerr << RED << "Error: User not found for fd " << userFd << RESET << std::endl;
		else
		{
			std::cout	<< "New user info: "
						<< "Nickname: " << YELLOW << getUser(userFd)->getNickname() << RESET
						<< ", Username: " << YELLOW << getUser(userFd)->getUsername() << RESET
						<< std::endl;
		}
	}
}

/**
 Retrieves a User object by its file descriptor (fd) in a safe manner.

 This method safely searches the `_users` map using `.find()` (instead of `[]`)
 to avoid accidental insertion of invalid keys.

 @param fd 	The file descriptor (socket) of the user to retrieve.
 @return 	Pointer to the `User` object if found, `NULL` otherwise.
*/
User*	Server::getUser(int fd) const
{
	std::map<int, User*>::const_iterator	it = _users.find(fd);
	if (it != _users.end())
		return it->second;
	return NULL;
}