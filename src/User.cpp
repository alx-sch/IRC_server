#include <iostream>
#include <string>

#include "../include/User.hpp"
#include "../include/Server.hpp"
#include "../include/utils.hpp"		// isLetter, isDigit, isSpecial
#include "../include/defines.hpp"	// color definitions

User::User(int fd, Server* server)
	:	_fd(fd), _nickname("*"), _server(server), _hasNick(false),
		_hasUser(false), _hasPassed(false), _isRegistered(false)
{}

User::~User() {}
User::User(int fd) : _fd(fd) {}

/////////////
// Setters //
/////////////

// This sets the file descriptor to -1, indicating the user is no longer connected.
void	User::markDisconnected()
{
	_fd = -1;
}

/**
 Attempts to set the user's nickname, performing the following checks:
 - Validates nickname syntax according to IRC rules
 - Ensures the nickname is not already in use

 On failure, sends the appropriate numeric error reply:

 On success:
 - Updates the nickname
 - Updates the server's nickname registry
 - Triggers registration completion if all fields are set
*/
void	User::setNickname(const std::string& nickname)
{
	// Check if the nickname is valid according to IRC rules
	if (!isValidNick(nickname))
	{
		std::cout	<< GREEN << getNickname() << RESET
					<< " (" << MAGENTA << "fd " << _fd << RESET
					<< ") tried to set an invalid nickname: "
					<< RED << nickname << RESET << std::endl;

		replyError(432, nickname, "Erroneous nickname");
		return;
	}

	// Nickname is already in use?
	if (_server->getNickMap().count(nickname) > 0)
	{
		std::cout	<< GREEN << getNickname() << RESET
					<< " (" << MAGENTA << "fd " << _fd << RESET << ") "
					<< "tried to set a nickname already in use: "
					<< YELLOW << nickname << RESET << std::endl;

		replyError(433, nickname, "Nickname is already in use");
		return;
	}

	// Valid and free — set the nickname; log the change in server terminal
	std::cout	<< GREEN << getNickname() << RESET
				<< " (" << MAGENTA << "fd " << _fd << RESET
				<< ") set (new) nickname: "
				<< CYAN << nickname << RESET << std::endl;

	// If the user already had a nickname, remove the old one
	if (!_nickname.empty())
		_server->removeNickMapping(_nickname);

	// Add the new nickname to the server's user map and update the user object
	_server->getNickMap()[nickname] = this;
	_nickname = nickname;
	_hasNick = true;
	tryRegister();
}

// Set the username for the user
void	User::setUsername(const std::string& username)
{
	_username = username;
	_hasUser = true;
	tryRegister();
}

// Set the real name for the user (usually unused)
void	User::setRealname(const std::string& realname)
{
	_realname = realname;
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

// Returns the input buffer where incoming messages are stored.
std::string&	User::getInputBuffer()
{
	return _inputBuffer;
}

///////////
// Utils //
///////////

/**
 Check if the nickname is valid according to IRC rules
 - Must not be empty
 - Max length is 9 characters
 - Must start with a letter
 - Can contain letters, digits, and special characters

 <nick> ::= <letter> { <letter> | <number> | <special> }
*/
bool	User::isValidNick(const std::string& nick)
{
	if (nick.empty() || nick.length() > 9)
		return false;

	if (!isLetter(nick[0])) // IRC rule: must start with a letter
		return false;

	for (unsigned int i = 1; i < nick.length(); ++i)
	{
		char	c = nick[i];
		if (!isLetter(c) && !isDigit(c) && !isSpecial(c))
			return false;
	}
	return true;
}
