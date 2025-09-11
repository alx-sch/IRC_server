#include <iostream>
#include <string>

#include "../include/User.hpp"
#include "../include/Server.hpp"
#include "../include/utils.hpp"		// logUserAction
#include "../include/defines.hpp"	// color formatting

// '*' is default nickname for unregistered users
User::User(int fd, Server* server)
	:	_fd(fd), _nickname("*"), _server(server), _hasNick(false),
		_hasUser(false), _hasPassed(false), _isRegistered(false),
		_sendErrorLogged(false)
{}

User::~User() {}

// Returns the prefix in the format: nickname!username@host
std::string	User::buildPrefix() const
{
	return _nickname + "!" + _username + "@" + _host;
}

// Returns whether a send error has been logged for this user.
bool	User::hasSendErrorLogged() const
{
	return _sendErrorLogged;

}

// Sets the flag indicating whether a send error has been logged for this user.
void	User::setSendErrorLogged(bool value)
{
	_sendErrorLogged = value;
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
void	User::setNickname(const std::string& nickname)
{
	logUserAction(_nickname, _fd, std::string("set nickname to ") + GREEN + nickname + RESET);

	// If the user already had a nickname, remove the old one
	if (!_nickname.empty())
		_server->removeNickMapping(_nickname);

	// Add the new nickname to the server's user map and update the user object
	_server->getNickMap()[nickname] = this;
	_nickname = nickname;
	_hasNick = true;
}

// Set the username for the user
void	User::setUsername(const std::string& username)
{
	_username = username;
	_hasUser = true;
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

/////////////
// Getters //
/////////////

// Returns the file descriptor (socket) for the user.
const int&	User::getFd() const
{
	return _fd;
}

// Returns the nickname of the user.
const std::string&	User::getNickname() const
{
		return _nickname;
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
	_channels.insert(channel);
}

// Removes a channel from the user's set of joined channels.
void	User::removeChannel(const std::string& channel)
{
	_channels.erase(channel);
}

