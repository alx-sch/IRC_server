#include <iostream>
#include <string>
#include <cstring>		// strerror()
#include <cerrno>		// errno
#include <stdexcept>	// std::runtime_error()

#include <unistd.h>		// close()
#include <sys/types.h>	// size_t, ssize_t
#include <sys/socket.h>	// accept(), recv(), send(), FD_* macros
#include <netinet/in.h>	// sockaddr_in, ntohs()
#include <arpa/inet.h>	// inet_ntoa()

#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/defines.hpp"
#include "../include/utils.hpp"	// toString()

///////////////////////////////
// Accepting Users on Server //
///////////////////////////////

/**
 Accepts a new user connection and adds them to the server's user lists
 (`_usersFd`, `_usersNick`) by creating a new `User` object.
*/
void	Server::acceptNewUser()
{
	int			userFd;		// fd for the accepted user connection
	sockaddr_in	userAddr;	// Init user address structure
	socklen_t	userLen = sizeof(userAddr);

	userFd = accept(_fd, reinterpret_cast<sockaddr*>(&userAddr), &userLen);
	if (userFd == -1)
		throw std::runtime_error("accept() failed: " + std::string(strerror(errno)));

	std::cout	<< "New connection from "
				<< YELLOW << inet_ntoa(userAddr.sin_addr)		// IP address as string
				<< ":" << ntohs(userAddr.sin_port)	<< RESET	// Port (convert from network order)
				<< " (" << MAGENTA << "fd " << userFd << RESET << ")\n";

	User* newUser = new User();
	_usersFd[userFd] = newUser;
}

///////////////////////////////////
// Input Handling & Broadcasting //
///////////////////////////////////

/**
 Handles input readiness for all connected users.

 This function iterates through all user file descriptors and checks if any are marked
 as ready for reading (based on `select()` populating `readFds`). For each ready user,
 it attempts to read and process input using `handleUserInput()`. If a user has disconnected
 or an error occurred while reading, the user is removed from the server via `deleteUser()`.

 @param readFds 	A set of file descriptors marked as ready to read by `select()`.
*/
void	Server::handleReadyUsers(fd_set& readFds)
{
	std::map<int, User*>::iterator	it = _usersFd.begin();

	// Iterate through all active users and check if they have data to read
	while (it != _usersFd.end())
	{
		int	userFd = it->first;
		++it;

		if (FD_ISSET(userFd, &readFds))
		{	
			if (!handleUserInput(userFd)) // User disconnected or error
				deleteUser(userFd);
		}
	}
}

/**
 Handles incoming data from a user socket.

 This function reads data from the specified fd into a temporary buffer,
 appends it to the user's persistent input buffer, and processes complete messages
 delimited by newline characters (`\n`). Each complete message is then broadcast to all other users.

 @param fd 	The fd of the user to read input from.
 @return 	`true` if input was successfully handled, `false` if the user disconnected or an error occurred.
*/
bool	Server::handleUserInput(int fd)
{
	User*		user = _usersFd[fd];
	char		buffer[MAX_BUFFER_SIZE];	// Temp buffer on the stack for incoming data
	ssize_t		bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);	// Read from user socket

	if (bytesRead <= 0)
	{
		// Log the disconnection / error immediately
		handleDisconnection(fd, (bytesRead == 0 ? "Connection closed" : strerror(errno)));
		return false; // User removed in `handleReadyUsers()`
	}

	// Append the received bytes to the user's input buffer
	user->getInputBuffer().append(buffer, bytesRead);

	extractMessagesFromBuffer(fd, user);

	return true;
}

void	Server::extractMessagesFromBuffer(int fd, User* user)
{
	std::string&	buffer = user->getInputBuffer();
	std::string		message;
	std::string		nick;
	size_t			newlinePos;

	while ((newlinePos = buffer.find('\n')) != std::string::npos)
	{
		message = buffer.substr(0, newlinePos);
		buffer.erase(0, newlinePos + 1);

		// You could also parse commands here (e.g. NICK, USER) in future
		nick = getUserNickSafe(fd);
		broadcastMessage(fd, nick, message);
	}
}

/**
 Broadcasts a message from one user to all other connected users.

 If a `send()` operation fails, it calls `handleSendError()` to log and handle the error.

 @param senderFd 	The fd of the user who sent the message.
 @param message 	The message to broadcast to all other users.
*/
void	Server::broadcastMessage(int senderFd, const std::string& nick, const std::string& message)
{
	// Format the message with color and sender nickname --> LIKELY HANDLED BY IRC CLIENT
	std::string output =	std::string(MAGENTA) + std::string(BOLD) + nick
							+ ": " + std::string(RESET) + message + "\n";

	// Loop through all connected users
	std::map<int, User*>::iterator	it = _usersFd.begin();
	while (it != _usersFd.end())
	{
		int targetFd = it->first;
		++it; // Advance iterator early in case target FD is removed

		if (targetFd != senderFd) // Don't send the message back to the sender
		{
			// Attempt to send the message to the target user
			if (send(targetFd, output.c_str(), output.length(), 0) == -1)
				handleSendError(targetFd, nick);
		}
	}
}

/**
 Handles a failed `send()` operation to a user.

 Logs the error to the server terminal.
 If the failure was due to a broken or reset connection (`EPIPE` or `ECONNRESET`),
 the user is considered disconnected and is removed from the server.

 @param fd 	The fd of the user for whom `send()` failed.
*/
void	Server::handleSendError(int fd, const std::string& nick)
{
	// Log the send error with color formatting
	std::cerr	<< RED << "Failed to send to "
				<< GREEN << nick << RED
				<< " (" << MAGENTA << "fd " << fd << RED << "): "
				<< strerror(errno) << RESET << std::endl;

	// If the error was a broken pipe or connection reset, treat it as a disconnect
	if (errno == EPIPE || errno == ECONNRESET)
	{
		// Log the disconnection, remove the user from the server
		handleDisconnection(fd, strerror(errno));
		deleteUser(fd);
	}
}

/**
 Logs a disconnection event for a user.

 This function is called when a user disconnects or an error occurs.
 It prints the user's nickname and fd, along with the reason for disconnection.

 @param fd 		The file descriptor of the disconnected user.
 @param reason 	The reason for disconnection (e.g., "Connection closed", "Broken pipe").
*/
void	Server::handleDisconnection(int fd, const std::string& reason)
{
	std::string	nick = getUserNickSafe(fd);

	std::cerr	<< RED << "Removing user "
				<< GREEN << nick << RED
				<< " (" << MAGENTA << "fd " << fd << RED << "): "
				<< reason << RESET << std::endl;
}

/////////////////////
// Get User (info) //
/////////////////////

/**
 Retrieves an `User` object by its file descriptor (fd) in a safe manner.

 This method safely searches the `_usersFd` map using `.find()` (instead of `[]`)
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
 Retrieves an `User` object by its nickname in a safe manner.

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

/**
 Retrieves the nickname of a user in a safe manner.

 If the user is not found or their nickname is empty, returns a default "Guest#<fd>" string.

 @param fd 	The file descriptor (socket) of the user to retrieve the nickname for.
 @return 	A safe nickname string.
*/
std::string	Server::getUserNickSafe(int fd) const
{
	User* user = getUser(fd);
	if (!user || user->getNickname().empty())
		return "Guest#" + toString(fd); // Return a safe default if user not found or nickname not set yet
	else
		return (user->getNickname());
}

//////////////////
// Remove Users //
//////////////////

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
