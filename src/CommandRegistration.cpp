#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/utils.hpp"		// isValidNick, normalize
#include "../include/defines.hpp"	// color formatting

#include <algorithm>	// For std::transform

// Handles the `NICK` command for a user. Also part of the initial client registration.
// Command: `NICK <nickname>`
void	Command::handleNick(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (tokens.size() < 2)
	{
		user->logUserAction("sent NICK without a nickname");
		user->sendError(431, "", "No nickname given");
		return;
	}

	const std::string&	displayNick = tokens[1];

	// Check if the nickname is valid according to IRC rules
	if (!isValidNick(displayNick))
	{
		user->logUserAction(toString("tried to set an invalid nickname: ")
			+ RED + displayNick + RESET);
		user->sendError(432, displayNick, "Erroneous nickname");
		return;
	}

	// Normalize the nickname for storage and lookup (case-insensitive)
	std::string	normNick = normalize(displayNick);

	// Nickname is already in use?
	if (server->getNickMap().count(normNick) > 0)
	{
		user->logUserAction(toString("tried to set a nickname already in use: ")
			+ YELLOW + displayNick + RESET);
		user->sendError(433, displayNick, "Nickname is already in use");
		return;
	}

	// Successfully change the nickname

	// If no username is set yet, use temp username
	if (user->getUsername().empty())
		user->setUsernameTemp("~" + displayNick);

	// Notify user of their own nick change
	user->sendMsgFromUser(user, "NICK :" + displayNick);
	user->setNickname(displayNick, normNick);
	user->tryRegister();

	// Notify other users of the nick change
	if (user->getChannels().size() > 0)
	{
		std::string	notice = ":" + user->buildHostmask() + " NICK :" + displayNick;
		for (std::set<std::string>::const_iterator it = user->getChannels().begin();
				it != user->getChannels().end(); ++it)
		{
			Channel*	channel = server->getChannel(*it);
			if (channel)
				broadcastToChannel(channel, notice, normNick); // Exclude the user changing nick
		}
	}
}

// Handles the `USER` command for a user. Also part of the initial client registration.
// Command: `USER <username> <hostname> <servername> :<realname>`
void	Command::handleUser(User* user, const std::vector<std::string>& tokens)
{
	if (user->isRegistered())
	{
		user->logUserAction("tried to resend USER after registration");
		user->sendError(462, "", "You may not reregister");
		return;
	}

	// Enough arguments?
	if (tokens.size() < 5)
	{
		user->logUserAction("sent invalid USER command (too few arguments)");
		user->sendError(461, "USER", "Not enough parameters");
		return;
	}

	user->logUserAction("sent valid USER command");

	// Set username and realname / ignore hostname and servername
	user->setUsername(tokens[1]);

	// Handle realname
	user->setRealname(tokens[4]);

	user->tryRegister();
}

// Handles the `PASS` command for a user. This is used to authenticate the user with the server.
// Command: `PASS <password>`
void	Command::handlePass(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (user->isRegistered())
	{
		user->logUserAction("tried to resend PASS after registration");
		user->sendError(462, "", "You may not reregister");
		return;
	}

	if (tokens.size() < 2)
	{
		user->logUserAction("sent invalid PASS command (missing password)");
		user->sendError(461, "PASS", "Not enough parameters");
		return;
	}

	// Validate the provided password
	// If server password is empty, any password is accepted
	const std::string&	password = tokens[1];
	bool				requiresPassword = !server->getPassword().empty();
	if (requiresPassword && password != server->getPassword())
	{
		user->logUserAction("provided incorrect password");
		user->sendError(464, "", "Password incorrect");
		return;
	}

	user->logUserAction("sent valid PASS command");
	user->setHasPassed(true);
	user->tryRegister();
}
