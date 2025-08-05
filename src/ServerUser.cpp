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
	if (userFd == -1) // Critical! Shut down server / end program
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
		logUserAction("*", userFd, RED
			+ std::string("ERROR: Failed to allocate memory for new user. Connection closed") + RESET);
		return; // Keep server running
	}
}

/////////////////////////
// Handling User Input //
/////////////////////////

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
		logUserAction(user->getNickname(), fd, RED + std::string("ERROR: recv() failed: ")
			+ strerror(errno) + RESET);
		return false;
	}

	// Append the received bytes to the user's input buffer
	user->getInputBuffer().append(buffer, bytesRead);

	messages = extractMessagesFromBuffer(user);

	// Process each complete message
	for (size_t i = 0; i < messages.size(); ++i)
	{
		if (!Command::handleCommand(this, user, messages[i]))
		{	
			std::vector<std::string>	tokens = Command::tokenize(messages[i]);
			std::string	cmd = tokens[0];

			logUserAction(user->getNickname(), fd, std::string("sent unknown command: ")
				+ RED + cmd + RESET);
			user->replyError(421, cmd, "Unknown command");
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

		// Check if line is too long (more than 510 + CRLF = 512); see RFC 1459, 2.3
		if (msg.size() > 510)
		{
			logUserAction(user->getNickname(), user->getFd(), "sent an overlong line ("
				+ toString(msg.size()) + " > 512 bytes)");
			user->replyError(417, "", "Input line was too long");
			continue; // Skip this message, do not add to vector
		}

		// Store the clean message in vector
		messages.push_back(msg);
	}
	return messages;
}

/**
 Logs and handles an unexpected disconnection event.

 This function is called when an I/O error occurs during communication
 (e.g., recv() or send() fails). It logs the user's nickname and file descriptor,
 along with the error source and reason.

 @param fd 		The file descriptor of the disconnected user.
 @param reason 	The reason for disconnection (e.g., "Connection closed", "Broken pipe").
 @param source 	A string indicating where the error occurred (e.g., "recv()", "send()").
*/
void	Server::handleDisconnection(int fd, const std::string& reason, const std::string& source)
{
	// Just for safety,in case User was already deleted
	User*		user = getUser(fd);
	std::string	nick = user ? user->getNickname() : "*";

	std::cerr	<< RED << BOLD << "Error: " << source << " failed for user "
				<< GREEN << nick << RED
				<< " (" << MAGENTA << "fd " << fd << RED << "): "
				<< reason << std::endl;
}

//////////////////////////
// Handling Ready Users //
//////////////////////////

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
			if (!handleUserInput(userFd))
			{
				if (errno == 0) // User disconnected without any error (e.g. CTRL + C)
					deleteUser(userFd, "disconnected (user logout)");
				else
					deleteUser(userFd, strerror(errno));
			}
		}
	}
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
			std::string&	outputBuffer = user->getOutputBuffer();
			ssize_t			bytesSent = send(userFd, outputBuffer.c_str(), outputBuffer.length(), 0);
			
			if (bytesSent == -1)
			{
				// Handle send error - may disconnect user
				handleSendError(userFd, user->getNickname());
				if (errno == EPIPE || errno == ECONNRESET)
					continue; // User was deleted by handleSendError
			}
			else if (bytesSent > 0)
			{
				outputBuffer.erase(0, bytesSent);	// Remove sent data from buffer
				user->setSendErrorLogged(false);	// Reset error log flag
			}
		}
	}
}

/**
 Handles a failed `send()` operation to a user.

 Logs the error to the server terminal.
 If the failure was due to a broken or reset connection (`EPIPE` or `ECONNRESET`),
 the user is considered disconnected and is removed from the server.

 @param fd 		The fd of the user for whom `send()` failed.
 @param nick 	The nickname of the user for whom `send()` failed.
*/
void	Server::handleSendError(int fd, const std::string& nick)
{
	User*	user = getUser(fd);

	// Critical: user is disconnected
	if (errno == EPIPE || errno == ECONNRESET)
	{
		handleDisconnection(fd, strerror(errno), "send()");
		deleteUser(fd, strerror(errno));
		return;
	}

	// Non‑critical send error (temporary): log it ONCE but do not disconnect
	// e.g. EAGAIN or EWOULDBLOCK (“Resource temporarily unavailable”)
	if (user && !user->hasSendErrorLogged())
	{
		logUserAction(nick, fd, RED + std::string("ERROR: sending message to client failed: ")
			+ strerror(errno) + RESET);
		user->setSendErrorLogged(true); // Prevent multiple logs for the same error
	}
}

//////////////
// Get User //
//////////////

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
void	Server::deleteUser(int fd, std::string logMsg)
{
	User*	user = getUser(fd);
	if (!user)
		return;

	// Log before we close and erase everything
	logUserAction(user->getNickname(), fd, logMsg);

	close(fd);
	user->markDisconnected();
	_usersFd.erase(fd);
	_usersNick.erase(user->getNickname());
	delete user;
}
