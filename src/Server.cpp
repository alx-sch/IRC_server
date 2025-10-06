#include <iostream>
#include <string>
#include <stdexcept>	// std::runtime_error
#include <cerrno>		// errno
#include <cstring>		// memset(), strerror()

#include <unistd.h>		// close()
#include <sys/select.h>	// select(), fd_set, FD_* macros

#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/defines.hpp"	// color formatting
#include "../include/signal.hpp"	// g_running variable
#include "../include/utils.hpp"		// getFormattedTime(), logServerMessage()

/// Constructor: Initializes the server socket and sets up the server state.
Server::Server(int port, const std::string& password) 
	:	_name(SERVER_NAME), _version(VERSION), _network(NETWORK),
		_creationTime(getFormattedTime()), _port(port),
		_password(password), _fd(-1), _cModes(C_MODES), _uModes(U_MODES),
		_maxChannels(MAX_CHANNELS), _botMode(false), _botFd(-1)
{
	initSocket();
}

// Destructor: Closes the server socket and cleans up resources.
Server::~Server()
{
	// Close the listening socket if open
	if (_fd != -1)
		close(_fd);

	if (_botFd != -1)
		close(_botFd);

	// Delete all dynamically allocated User objects
	while (!_usersFd.empty())
		deleteUser(_usersFd.begin()->first, toString("disconnected (") + YELLOW + "server shutdown" + RESET + ")");

	// Delete all dynamically allocated Channel objects
	while (!_channels.empty())
		deleteChannel(_channels.begin()->first, "server shutdown");

	logServerMessage("Server shutdown complete");
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
	fd_set	readFds, writeFds;	// Sets of fds to monitor for readability and writability
	int		maxFd;		// Highest fd in the set, used by select() to avoid scanning all fds
	int		writeMaxFd;	// Highest fd in the write set
	int		ready;		// Number of ready fds returned by select()

	// Initializes the bot if bot mode is set.
	#ifdef BOT_MODE
		initBot();
	#endif

	while (g_running)
	{
		maxFd = prepareReadSet(readFds);
		writeMaxFd = prepareWriteSet(writeFds);
		if (writeMaxFd > maxFd) maxFd = writeMaxFd;

		// Pause the program until a socket becomes readable or writable imn any of the provided sets
		// 'exceptional' set is not used (NULL), also no timeour set (NULL)
		ready = select(maxFd + 1, &readFds, &writeFds, NULL, NULL);
		if (ready == -1) // Critical! Shut down server / end program
		{
			if (errno == EINTR) // If interrupted by signal (SIGINT), just return to main.
				return;
			throw std::runtime_error("select() failed: " + toString(strerror(errno)));
		}

		// New incoming connection?
		if (FD_ISSET(_fd, &readFds)) // checks if server socket (_fd) is ready for reading -> new connection
			acceptNewUser(); // Adds user to `_usersFd`

		// Handle user input for all active connections (messages, disconnections)
		handleReadReadyUsers(readFds);
		
		// Handle pending output to be sent to users
		handleWriteReadyUsers(writeFds);
	}
}

/////////////
// Getters //
/////////////

// Returns the server name.
const std::string&	Server::getServerName() const
{
	return _name;
}

// Returns the server version.
const std::string&	Server::getVersion() const
{
	return _version;
}

// Returns the network name.
const std::string&	Server::getNetwork() const
{
	return _network;
}

// Returns the server creation time.
const std::string&	Server::getCreationTime() const
{
	return _creationTime;
}

// Returns the server password.
const std::string&	Server::getPassword() const
{
	return _password;
}

// Returns the server port.
int	Server::getPort() const
{
	return _port;
}

// Returns the channel modes string.
const std::string&	Server::getCModes() const
{
	return _cModes;
}

// Returns the user modes string.
const std::string&	Server::getUModes() const
{
	return _uModes;
}

// Returns the maximum number of channels a user can join.
int	Server::getMaxChannels() const
{
	return _maxChannels;
}

//////////////////
// Nick Mapping //
//////////////////

// Returns a map of active users by nickname.
std::map<std::string, User*>&	Server::getNickMap()
{
	return _usersNick;
}

// Removes a nickname mapping from the server's user map.
// Used when user changes their nickname.
void	Server::removeNickMapping(const std::string& nickname)
{
	_usersNick.erase(nickname);
}
