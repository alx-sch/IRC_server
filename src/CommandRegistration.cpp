#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidNick
#include "../include/defines.hpp"	// color formatting

// Handles the `NICK` command for a user. Also part of the initial client registration.
// Command: `NICK <nickname>`
bool	Command::handleNick(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent NICK without a nickname");
		user->replyError(431, "", "No nickname given");
		return false;
	}

	const std::string&	nick = tokens[1];

	// Check if the nickname is valid according to IRC rules
	if (!isValidNick(nick))
	{
		logUserAction(user->getNickname(), user->getFd(), std::string("tried to set an invalid nickname: ")
			+ RED + nick + RESET);
		user->replyError(432, nick, "Erroneous nickname");
		return false;
	}

	// Nickname is already in use?
	if (server->getNickMap().count(nick) > 0)
	{
		logUserAction(user->getNickname(), user->getFd(),
			std::string("tried to set a nickname already in use: ") + YELLOW + nick + RESET);
		user->replyError(433, nick, "Nickname is already in use");
		return false;
	}

	user->setNickname(nick);
	user->tryRegister();

	return true;
}

// Handles the `USER` command for a user. Also part of the initial client registration.
// Command: `USER <username> <hostname> <servername> :<realname>`
bool	Command::handleUser(User* user, const std::vector<std::string>& tokens)
{
	if (user->isRegistered())
	{
		logUserAction(user->getNickname(), user->getFd(), "tried to resend USER after registration");
		user->replyError(462, "", "You may not reregister");
		return false;
	}

	// Enough arguments? Does realname token start with a colon?
	if (tokens.size() < 5 || tokens[4].size() < 2 || tokens[4][0] != ':')
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid USER command (too few arguments)");
		user->replyError(461, "USER", "Not enough parameters");
		return false;
	}
	
	logUserAction(user->getNickname(), user->getFd(), "sent valid USER command");
	user->setUsername(tokens[1]);
	user->setHost(tokens[2]);
	user->setRealname(tokens[4].substr(1)); // Remove leading colon from realname
	user->tryRegister();

	return true;
}

// Handles the `PASS` command for a user. This is used to authenticate the user with the server.
// Command: `PASS <password>`
bool	Command::handlePass(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (user->isRegistered())
	{
		logUserAction(user->getNickname(), user->getFd(), "tried to resend PASS after registration");
		user->replyError(462, "", "You may not reregister");
		return false;
	}

	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid PASS command (missing password)");
		user->replyError(461, "PASS", "Not enough parameters");
		return false;
	}

	// Validate the provided password
	// If server password is empty, any password is accepted
	const std::string&	password = tokens[1];
	bool				requiresPassword = !server->getPassword().empty();
	if (requiresPassword && password != server->getPassword())
	{
		logUserAction(user->getNickname(), user->getFd(), "provided incorrect password");
		user->replyError(464, "", "Password incorrect");
		return false;
	}

	logUserAction(user->getNickname(), user->getFd(), "sent valid PASS command");
	user->setHasPassed(true);
	user->tryRegister();
	
	return true;
}
