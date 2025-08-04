#include <iostream>
#include <string>
#include <cstring>		// strerror()
#include <cerrno>		// errno
#include <stdexcept>	// std::runtime_error()
#include <map>
#include <vector>

#include <unistd.h>		// close()
#include <sys/types.h>	// size_t, ssize_t
#include <sys/socket.h>	// accept(), recv(), send(), FD_* macros
#include <netinet/in.h>	// sockaddr_in, ntohs()
#include <arpa/inet.h>	// inet_ntoa()

#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Command.hpp"
#include "../include/defines.hpp"
#include "../include/utils.hpp"	// toString(), logUserAction

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

	logUserAction("*", userFd, std::string("connected from ") + YELLOW
		+ std::string(inet_ntoa(userAddr.sin_addr)) + RESET);

	try
	{
		User* newUser = new User(userFd, this); // 'new' throws std::bad_alloc on failure
		_usersFd[userFd] = newUser;
	}
	catch(const std::bad_alloc&)
	{
		close(userFd);	// Close fd to prevent leak
		logServerMessage(RED + std::string("ERROR: Failed to allocate memory for new user on ")
			+ MAGENTA + "fd " + toString(userFd) + RED ". Connection closed." + RESET);
		return; // Keep server running
	}
}

/////////////////////////
// Handling User Input //
/////////////////////////

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
	char						buffer[MAX_BUFFER_SIZE]; // Temp buffer on the stack for incoming data
	ssize_t						bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0); // Read from user socket
	std::vector<std::string>	messages;
	User*						user = getUser(fd);
	if (!user)
		return false;

	if (bytesRead == 0) // Connection closed by the user
		return false;

	if (bytesRead < 0) // recv() failed (bytesRead == -1)
	{
		handleDisconnection(fd, strerror(errno), "recv()");
		return false;
	}

	// Append the received bytes to the user's input buffer
	user->getInputBuffer().append(buffer, bytesRead);

	messages = extractMessagesFromBuffer(user);

	// Process each complete message
	for (size_t i = 0; i < messages.size(); ++i)
	{
		if (!Command::handleCommand(this, user, messages[i]))
		{	// Just for testing, in a proper IRC implementation, below would be a "Command unknown" response
			// (Also broadcast / channel messages have a command prefix)
			// prob return false here when it's "kickable" user behavior
			std::string	nick = user->getNickname();
			broadcastMessage(fd, nick, messages[i]); // allows testing without full IRC client (works with telnet, nc)
		}
	}

	return true;
}

/**
Extracts complete IRC messages from the user's input buffer.

 @param user 	Pointer to the user whose input buffer is being processed.
 @return 		A vector of complete, cleaned IRC messages.
*/
std::vector<std::string>	Server::extractMessagesFromBuffer(User* user)
{
	std::string&				buffer = user->getInputBuffer(); // Reference to the user's input buffer
	std::vector<std::string>	messages;
	std::string					msg;
	std::string					nick;
	size_t						newlinePos;

	// Extract complete messages (terminated by '\n')
	while ((newlinePos = buffer.find('\n')) != std::string::npos)
	{
		// Get the next full line (up to the newline)
		msg = buffer.substr(0, newlinePos);

		// Remove it from the input buffer (including the newline)
		buffer.erase(0, newlinePos + 1);

		// Handle optional carriage return (\r) for \r\n line endings
		// (as per IRC spec for server-client communication)
		if (!msg.empty() && msg[msg.size() - 1] == '\r')
			msg.erase(msg.size() - 1);

		// Store the clean message in vector
		messages.push_back(msg);
	}
	return messages;
}

/** TESTING! WON'T BE IN FINAL VERSION (chat via terminal: telnet, nc)
 Broadcasts a message from one user to all other connected users.

 If a `send()` operation fails, it calls `handleSendError()` to log and handle the error.

 @param senderFd 	The fd of the user who sent the message.
 @param message 	The message to broadcast to all other users.
*/
void	Server::broadcastMessage(int senderFd, const std::string& nick, const std::string& message)
{
	// Format the message with color and sender nickname --> LIKELY HANDLED BY IRC CLIENT
	std::string	output = nick + ": " + message + "\n";

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

/** PROB MOVE TO COMMAND --> WHEN HANDLING MESSAGES THERE
 Handles a failed `send()` operation to a user.

 Logs the error to the server terminal.
 If the failure was due to a broken or reset connection (`EPIPE` or `ECONNRESET`),
 the user is considered disconnected and is removed from the server.

 @param fd 	The fd of the user for whom `send()` failed.
*/
void	Server::handleSendError(int fd, const std::string& nick)
{
	// Critical errors that mean the user is gone
	if (errno == EPIPE || errno == ECONNRESET)
	{
		handleDisconnection(fd, strerror(errno), "send()");
		deleteUser(fd);
		return;
	}

	// Non‑critical send error (temporary): log it but do not disconnect
	// e.g. EAGAIN or EWOULDBLOCK (“Resource temporarily unavailable”)
	logUserAction(nick, fd, RED + std::string("Error sending message to user: ")
		+ strerror(errno) + RESET);
}

/**
 Logs a disconnection event for a user.

 This function is called when a user disconnects or an error occurs.
 It prints the user's nickname and fd, along with the reason for disconnection.

 @param fd 		The file descriptor of the disconnected user.
 @param reason 	The reason for disconnection (e.g., "Connection closed", "Broken pipe").
 @param source 	The source of the disconnection (who called this function).
*/
void	Server::handleDisconnection(int fd, const std::string& reason, const std::string& source)
{
	// Just for safety,in case User was already deleted
	User*		user = getUser(fd);
	std::string	nick = user ? user->getNickname() : "*";

	std::cerr	<< RED << BOLD << "Error: " << source << " failed for user "
				<< GREEN << nick << RED
				<< " (" << MAGENTA << "fd " << fd << RED << "): "
				<< reason << std::endl
				<< "Removing user from server.\n" << RESET;
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

//////////////////
// Remove Users //
//////////////////

// Deletes a user from the server (`_usersFd`, `_usersNick`) using their file descriptor.
void	Server::deleteUser(int fd)
{
	User*	user = getUser(fd);
	if (!user)
		return;

	// Log before we close and erase everything
	logUserAction(user->getNickname(), fd, "disconnected");

	close(fd);
	user->markDisconnected();
	_usersFd.erase(fd);
	_usersNick.erase(user->getNickname());
	delete user;
}

// Deletes a user from the server (`_usersFd`, `_usersNick`) using their nickname.
void	Server::deleteUser(const std::string& nickname)
{
	User*	user = getUser(nickname);
	if (!user)
		return;

	// Log before we close and erase everything
	logUserAction(user->getNickname(), user->getFd(), "disconnected");

	close(user->getFd());
	user->markDisconnected();
	_usersNick.erase(nickname);
	_usersFd.erase(user->getFd());
	delete user;
}

/**
 Handles output readiness for all connected users.

 This function iterates through all user file descriptors and checks if any are marked
 as ready for writing (based on `select()` populating `writeFds`). For each ready user
 with data in their output buffer, it attempts to send the data. If sending fails,
 the user may be disconnected based on the error type.

 @param writeFds 	A set of file descriptors marked as ready to write by `select()`.
*/
void	Server::handleWriteReadyUsers(fd_set& writeFds)
{
	std::map<int, User*>::iterator	it = _usersFd.begin();

	// Iterate through all active users and check if they're ready for writing
	while (it != _usersFd.end())
	{
		int		userFd = it->first;
		User*	user = it->second;
		++it;

		if (FD_ISSET(userFd, &writeFds) && user && !user->getOutputBuffer().empty())
		{
			std::string& outputBuffer = user->getOutputBuffer();
			ssize_t bytesSent = send(userFd, outputBuffer.c_str(), outputBuffer.length(), 0);
			
			if (bytesSent == -1)
			{
				// Handle send error - may disconnect user
				handleSendError(userFd, user->getNickname());
				if (errno == EPIPE || errno == ECONNRESET)
					continue; // User was deleted by handleSendError
			}
			else if (bytesSent > 0)
			{
				// Remove sent data from buffer
				outputBuffer.erase(0, bytesSent);
			}
			// If bytesSent == 0, no data was sent (shouldn't happen with select)
		}
	}
}
