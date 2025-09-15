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
#include "../include/Channel.hpp"
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
		throw std::runtime_error("accept() failed: " + toString(strerror(errno)));

	std::string	userIp = inet_ntoa(userAddr.sin_addr);

	logUserAction("*", userFd, toString("connected from ") + YELLOW
		+ toString(userIp) + RESET);

	try
	{
		User*	newUser = new User(userFd, this); // 'new' throws std::bad_alloc on failure
		_usersFd[userFd] = newUser;
		newUser->setHost(userIp);

		// Set as "password-passed" when server requires no password
		if (_password.empty())
			newUser->setHasPassed(true);
	}
	catch(const std::bad_alloc&)
	{
		close(userFd);	// Close fd to prevent leak
		logUserAction("*", userFd, RED
			+ toString("ERROR: Failed to allocate memory for new user. Connection closed") + RESET);
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

 @param fd		The fd of the user to read input from.
 @return		`true` if input was successfully handled,
				`false` if the user disconnected or an error occurred.
*/
Server::UserInputResult	Server::handleUserInput(int fd)
{
	char						buffer[MAX_BUFFER_SIZE]; // Temp buffer on the stack for incoming data
	ssize_t						bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0); // Read from user socket
	std::vector<std::string>	messages;
	User*						user = getUser(fd);

	if (!user)
		return INPUT_ERROR; // Should never happen, but just in case

	if (bytesRead == 0) // Connection closed by the user
		return INPUT_DISCONNECTED;

	if (bytesRead < 0) // recv() failed (bytesRead == -1)
	{
		// Check if non-fatal, temporary errors
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return INPUT_OK; // No data available right now, try again later
		
		// Otherwise, it's a fatal error
		return INPUT_ERROR;
	}	

	// Append the received bytes to the user's input buffer
	user->getInputBuffer().append(buffer, bytesRead);

	messages = extractMessagesFromBuffer(user);

	// Process each complete message
	for (size_t i = 0; i < messages.size(); ++i)
	{
		std::vector<std::string>	tokens = Command::tokenize(messages[i]);
		if (tokens.empty())
			continue; // Skip empty/space-only lines
		if (!Command::handleCommand(this, user, tokens))
		{
			std::string	cmd = tokens[0];

			logUserAction(user->getNickname(), fd, toString("sent unknown command: ")
				+ RED + cmd + RESET);
			user->replyError(421, cmd, "Unknown command");
		}
	}
	return INPUT_OK;
}

/**
Extracts complete IRC messages from the user's input buffer.

 @param user	Pointer to the user whose input buffer is being processed.
 @return		A vector of complete, cleaned IRC messages.
*/
std::vector<std::string>	Server::extractMessagesFromBuffer(User* user)
{
	std::string&				buffer = user->getInputBuffer(); // Reference to the user's input buffer
	std::vector<std::string>	messages;
	std::string					msg;
	size_t						newlinePos;

	// Extract complete messages (terminated by '\n')
	// Technically, IRC messages end with CRLF (\r\n), but many clients
	// just use LF (\n); also makes usage with terminal tools like netcat easier.
	// We will handle optional '\r' when processing each line below.
	// Postel's Law: Be conservative in what you send, liberal in what you accept.
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
			logUserAction(user->getNickname(), user->getFd(), toString("sent an overlong line (")
				+ YELLOW + toString(msg.size()) + RESET + " > 512 bytes)");
			user->replyError(417, "", "Input line was too long");
			continue; // Skip this message, do not add to vector
		}

		// Store the clean message in vector
		messages.push_back(msg);
	}
	return messages;
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

 @param readFds	A set of file descriptors marked as ready to read by `select()`.
*/
void	Server::handleReadReadyUsers(fd_set& readFds)
{
	std::map<int, User*>::iterator	it = _usersFd.begin();

	// Iterate through all active users and check if they have data to read
	while (it != _usersFd.end())
	{
		int	userFd = it->first;
		++it;

		if (FD_ISSET(userFd, &readFds))
		{
			UserInputResult	result = handleUserInput(userFd);
			if (result == INPUT_DISCONNECTED)
				disconnectUser(userFd, "Connection closed");
			else if (result == INPUT_ERROR)
			{
				logUserAction(getUser(userFd)->getNickname(), userFd, RED
					+ toString("ERROR: recv() failed: ") + toString(strerror(errno)) + RESET);
				disconnectUser(userFd, "Read error: " + toString(strerror(errno)));
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

 @param writeFds	A set of file descriptors marked as ready to write by `select()`.
*/
void	Server::handleWriteReadyUsers(fd_set& writeFds)
{
	std::map<int, User*>::iterator	it = _usersFd.begin();

	// Iterate through all active users and check if they're ready for writing
	while (it != _usersFd.end())
	{
		int		userFd = it->first;
		User*	user = it->second;
		++it;	// go to next user in map in advance

		if (FD_ISSET(userFd, &writeFds) && user && !user->getOutputBuffer().empty())
		{
			std::string&	outputBuffer = user->getOutputBuffer(); // output buffer: What the server has prepared to send to client
			ssize_t			bytesSent = send(userFd, outputBuffer.c_str(), outputBuffer.length(), 0);

			if (bytesSent > 0) // Successfully sent some data. Remove it from the buffer.
				outputBuffer.erase(0, bytesSent);
			else if (bytesSent == -1) // send() failed
			{
				if (errno == EPIPE || errno == ECONNRESET)
				{
					logUserAction(user->getNickname(), userFd, RED + toString("ERROR: send() failed: ")
						+ toString(strerror(errno)) + RESET);
					disconnectUser(userFd, "Write error: " + toString(strerror(errno)));
				}
				// If errno is EAGAIN or EWOULDBLOCK, do nothing (temporary issue), just try again in next select() loop
			}
		}
	}
}

//////////////
// Get User //
//////////////

/**
Retrieves an `User` object by its file descriptor (fd) in a safe manner.

This method safely searches the `_usersFd` map using `.find()` (instead of `[]`)
to avoid accidental insertion of invalid keys.

 @param fd	The file descriptor (socket) of the user to retrieve.
 @return	Pointer to the `User` object if found, `NULL` otherwise.
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

 @param nickname	The nickname of the user to retrieve.
 @return			Pointer to the `User` object if found, `NULL` otherwise.
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

	std::string	nick = user->getNickname();

	// Log before we close and erase everything
	logUserAction(nick, fd, logMsg);

	close(fd);
	user->markDisconnected();
	_usersFd.erase(fd);
	_usersNick.erase(nick);
	delete user;
}

/**
Handles the full disconnection process for a user.
This is the central point for all disconnections (quit and error).
It broadcasts the QUIT message and cleans up all server resources.
*/
void Server::disconnectUser(int fd, const std::string& reason)
{
	User*	user = getUser(fd);
	if (!user) return; // User already disconnected

	std::string	userNick = user->getNickname();
	std::string	quitMsg = ":" + user->buildHostmask() + " QUIT :" + reason;

	// Build unique set of users to notify
	std::set<User*>					recipients;
	const std::set<std::string>&	channels = user->getChannels();

	for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		Channel*	channel = getChannel(*it);
		if (channel)
		{
			const std::set<std::string>&	members = channel->get_members();
			for (std::set<std::string>::const_iterator mem_it = members.begin(); mem_it != members.end(); ++mem_it)
			{
				User*	member = getUser(*mem_it);
				if (member)
					recipients.insert(member);
			}
		}
	}
	recipients.erase(user); // Don't send QUIT to the user who is quitting

	// Broadcast the quit message
	for (std::set<User*>::iterator it = recipients.begin(); it != recipients.end(); ++it)
		(*it)->getOutputBuffer() += quitMsg + "\r\n";

	// Now, remove the user from all channels they were in
	for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		Channel*	channel = getChannel(*it);
		if (channel)
			channel->remove_user(userNick);
	}

	// Finally, delete the user from the server
	deleteUser(fd, toString("disconnected: ") + YELLOW + reason + RESET);
}
