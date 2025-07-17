
// C++ standard headers
#include <iostream>	
#include <stdexcept>	// std::runtime_error()
#include <cstring>		// memset(), strerror()
#include <cerrno>		// errno
#include <unistd.h>		// close()

// Socket and networking headers
#include <arpa/inet.h>	// inet_ntoa(), htons(), ntohs()
#include <netinet/in.h>	// sockaddr_in, INADDR_ANY
#include <sys/select.h> // for select(), fd_set, FD_* macros
#include <sys/socket.h>	// socket(), bind(), listen(), accept(), setsockopt(), etc.

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
	while (!_usersFd.empty())
		deleteUser(_usersFd.begin()->first);
}

/////////////
// Methods //
/////////////

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
		throw std::runtime_error("Failed to bind server socket to port " + toString(_port) + ": " + strerror(errno));
	}

	// Start listening for incoming connections (max connections as allowed by system)
	if (listen(_fd, SOMAXCONN) == -1)
	{
		close(_fd);
		throw std::runtime_error("Failed to listen on server socket: " + std::string(strerror(errno)));
	}
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
			throw std::runtime_error("select() failed: " + std::string(strerror(errno)));

		// Check if the server socket (_fd) is readable -> listening socket has a new connection
		if (FD_ISSET(_fd, &readFds))
			acceptNewUser();
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

/**
 Accepts a new user connection and adds them to the server's user lists
 (`_usersFd`, `_usersNick`) by creating a new User object.
*/
void	Server::acceptNewUser()
{
	int			userFd;		// fd for the accepted user connection
	sockaddr_in	userAddr;	// Init user address structure
	socklen_t	userLen = sizeof(userAddr);

	userFd = accept(_fd, reinterpret_cast<sockaddr*>(&userAddr), &userLen);
	if (userFd == -1)
		throw std::runtime_error("accept() failed: " + std::string(strerror(errno)));
	

	std::cout	<< "User "<< YELLOW << "#" << userFd << RESET << " connected from "
				<< YELLOW << inet_ntoa(userAddr.sin_addr)	// IP address as string
				<< ":" << ntohs(userAddr.sin_port)			// Port (convert from network order)
				<< RESET << std::endl;

	User* newUser = new User();
	_usersFd[userFd] = newUser;
}

// Deletes a user from the server (`_usersFd`, `_usersNick`) using their file descriptor.
void	Server::deleteUser(int fd)
{
	User* user = getUser(fd);
	if (!user)
		return;

	close(fd);
	_usersFd.erase(fd);
	_usersNick.erase(user->getNickname());
	delete user;
}

// Deletes a user from the server (`_usersFd`, `_usersNick`) using their nickname.
void	Server::deleteUser(const std::string& nickname)
{
	User* user = getUser(nickname);
	if (!user)
		return;

	close(user->getFd());
	_usersNick.erase(nickname);
	_usersFd.erase(user->getFd());
	delete user;
}

/////////////
// Getters //
/////////////

/**
 Retrieves a User object by its file descriptor (fd) in a safe manner.

 This method safely searches the `_users` map using `.find()` (instead of `[]`)
 to avoid accidental insertion of invalid keys.

 @param fd 	The file descriptor (socket) of the user to retrieve.
 @return 	Pointer to the `User` object if found, `NULL` otherwise.
*/
User*	Server::getUser(int fd) const
{
	std::map<int, User*>::const_iterator	it = _usersFd.find(fd);
	if (it != _usersFd.end())
		return it->second;
	return NULL;
}

/**
 Retrieves a User object by its nickname in a safe manner.

 This method safely searches the `_usersNick` map using `.find()` (instead of `[]`)
 to avoid accidental insertion of invalid keys.

 @param nickname 	The nickname of the user to retrieve.
 @return 			Pointer to the `User` object if found, `NULL` otherwise.
*/
User*	Server::getUser(const std::string& nickname) const
{
	std::map<std::string, User*>::const_iterator	it = _usersNick.find(nickname);
	if (it != _usersNick.end())
		return it->second;
	return NULL;
}
