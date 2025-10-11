#include <iostream>
#include <string>
#include <iomanip>		// std::setw
#include <sstream>		// std::ostringstream

#include "../include/User.hpp"
#include "../include/Server.hpp"
#include "../include/defines.hpp"	// color formatting
#include "../include/utils.hpp"		// getTimestamp(), toString()

// '*' is default nickname for unregistered users
User::User(int fd, Server* server)
	:	_fd(fd), _nickname("*"), _server(server), _hasNick(false),
		_hasUser(false), _hasPassed(false), _isRegistered(false), _isBot(false)
{}

User::~User() {}

// Returns the hostmask in the format: nickname!username@host
std::string	User::buildHostmask() const
{
	return _nickname + "!" + _username + "@" + _host;
}

/**
Formats a log line with timestamp, aligned nickname and fd columns.

 @param nick	The user's nickname
 @param fd		The user's socket fd
 @param message	The message to log
 @param botMode	True if bot mode is active (to color bot messages differently)
*/
void	User::logUserAction(const std::string& message, bool botMode)
{
	std::string			logColor = GREEN;
	std::ostringstream	fileLogEntry;

	if (botMode)
		logColor = BOT_COLOR;

	std::cout	<< "[" << CYAN << getTimestamp() << RESET << "] "
				<< logColor << std::left << std::setw(MAX_NICK_LENGTH + 1) << _nickname << RESET // pad nick + some space
				<< "(" << MAGENTA << "fd " << std::right << std::setw(3) << _fd << RESET << ") "
				<< message << std::endl;

	fileLogEntry	<< "[" << getTimestamp() << "] "
					<< std::left << std::setw(MAX_NICK_LENGTH + 1) << _nickname
					<< "(fd " << std::right << std::setw(3) << _fd << ") "
					<< removeColorCodes(message) << "\n";


	if (_server->getLogFile().is_open())
	{
		_server->getLogFile() << fileLogEntry.str();
		_server->getLogFile().flush(); // Ensure the data is written immediately to disk
	}

	
}

/////////////
// Setters //
/////////////

// This sets the file descriptor to -1, indicating the user is no longer connected.
void	User::markDisconnected()
{
	_fd = -1;
}

/**
Sets the user's nickname and updates server state accordingly.

This function assumes that the nickname has already been validated
for syntax and uniqueness by the caller (e.g., in the command handler).
*/
void	User::setNickname(const std::string& displayNick, const std::string& normNick)
{
	std::string	nickColor = GREEN;

	if (_isBot)
		nickColor = BOT_COLOR;

	logUserAction(toString("set nickname to ") + nickColor + displayNick + RESET, _isBot);

	// If the user already had a nickname, remove the old one
	if (_hasNick)
		_server->removeNickMapping(_nickname);

	// Add the new nickname to the server's user map and update the user object
	_server->getNickMap()[normNick] = this;
	_nickname = displayNick;
	_nicknameLower = normNick;
	_hasNick = true;
}

// Set the username for the user
void	User::setUsername(const std::string& username)
{
	_username = username;
	_hasUser = true;
}

// Set the username for the user temporarily (when NICK is set before USER)
void	User::setUsernameTemp(const std::string& username)
{
	_username = username;
}

// Set the real name for the user (usually unused)
void	User::setRealname(const std::string& realname)
{
	_realname = realname;
}

// Set the host for the user, most clients send '*' via USER command
void	User::setHost(const std::string& host)
{
	_host = host;
}

// Sets the _isBot variable to true
void	User::setIsBotToTrue(void)
{
	_isBot = true;
}

/////////////
// Getters //
/////////////

// Returns the file descriptor (socket) for the user.
int	User::getFd() const
{
	return _fd;
}

// Returns the nickname of the user.
const std::string&	User::getNickname() const
{
		return _nickname;
}

// Returns the normalized nickname of the user.
const std::string&	User::getNicknameLower() const
{
		return _nicknameLower;
}

// Returns the username of the user.
const std::string&	User::getUsername() const
{
	return _username;
}

// Returns the real name of the user.
const std::string&	User::getRealname() const
{
	return _realname;
}

// Returns the host of the user.
const std::string&	User::getHost() const
{
	return _host;
}

// Returns a pointer to the server the user is connected to.
const Server*	User::getServer() const
{
	return _server;
}

// Returns the input buffer where incoming messages are stored.
std::string&	User::getInputBuffer()
{
	return _inputBuffer;
}

// Returns the output buffer where outgoing messages are queued.
std::string&	User::getOutputBuffer()
{
	return _outputBuffer;
}

// True if user is IRCbot.
bool	User::getIsBot() const
{
	return _isBot;
}

////////////////////////
// Channel management //
////////////////////////

// Returns a const reference to the set of channel names the user is a member of.
const std::set<std::string>&	User::getChannels() const
{
	return _channels;
}

// Adds a channel to the user's set of joined channels.
void	User::addChannel(const std::string& channel)
{
	_channels.insert(normalize(channel));
}

// Removes a channel from the user's set of joined channels.
void	User::removeChannel(const std::string& channel)
{
	_channels.erase(normalize(channel));
}

