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

Server::Server(int port, const std::string& password) 
	:	_name(SERVER_NAME), _version(VERSION), _network(NETWORK),
		_creationTime(getFormattedTime()), _port(port),
		_password(password), _fd(-1), _cModes(C_MODES), _uModes(U_MODES),
		_maxChannels(MAX_CHANNELS)
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
	int		maxFd;		// Highest fd in the set
	int		writeMaxFd;	// Highest fd in the write set
	int		ready;		// Number of ready fds returned by select()

	logServerMessage("Server running on port " + toString(_port));

	while (g_running)
	{
		maxFd = prepareReadSet(readFds);
		writeMaxFd = prepareWriteSet(writeFds);
		if (writeMaxFd > maxFd) maxFd = writeMaxFd;

		// Pause the program until a socket becomes readable or writable
		ready = select(maxFd + 1, &readFds, &writeFds, NULL, NULL);
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
		
		// Handle user output for all users with pending data
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

void	Server::addChannel(Channel* channel)
{
	if (channel)
		_channels[channel->get_name()] = channel;
}

/**
 Retrieves an `Channel` object by its name.

 @param channelName 	The name of the channel to retrieve.
 @return				Pointer to the `Channel` object if found, `NULL` otherwise.
*/
Channel*	Server::getChannel(const std::string& channelName) const
{
	std::map<std::string, Channel*>::const_iterator	it = _channels.find(channelName);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

/**
 Retrieves an existing `Channel` by name or creates a new one if it does not exist.

 If the channel with the given name exists, a pointer to it is returned.
 Otherwise, a new `Channel` object is created, added to the server, and returned.
 In case of memory allocation failure, an error is logged and the user is notified.

 @param channelName 	The name of the channel to retrieve or create.
 @param user 			The user requesting or triggering the channel creation.
 @param key 			Optional channel key to set if creating a new channel.
 @return				Pointer to the `Channel` object, or `NULL` on failure.
*/
Channel*	Server::getOrCreateChannel(const std::string& channelName, User* user, const std::string& key)
{
	Channel*	channel = getChannel(channelName);
	if (channel)
		return channel;

	// Create a new channel if it does not exist
	try
	{
		channel = new Channel(channelName);
		if (!key.empty()) // If a key is provided, set it as the channel password
			channel->set_password(key);
		addChannel(channel);

		logUserAction(user->getNickname(), user->getFd(),
			std::string("created and joined new channel: ") + BLUE + channelName + RESET);
	}
	catch(const std::bad_alloc&)
	{
		logServerMessage(RED + std::string("ERROR: Failed to allocate memory for new channel ")
			+ BLUE + channelName + RED + " (created by " + GREEN + user->getNickname() 
			+ RED + ", " + MAGENTA + "fd " + toString(user->getFd()) + RED + ")" + RESET);

		user->replyError(500, "", "Internal server error while creating channel " + channelName);
		return NULL;
	}
	return channel;
}
