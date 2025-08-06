#include <string>
#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidChannelName
#include "../include/defines.hpp"	// color formatting

/////////////
// PRIVMSG //
/////////////

/**
 Sends a PRIVMSG from a user to a channel.

 @param server 			Pointer to the server instance.
 @param sender 			Pointer to the user sending the message.
 @param channelName 	Name of the target channel.
 @param message 		The message content to send.
*/
static void	handlePrivmsgToChannel(Server* server, User* sender, const std::string& channelName,
	const std::string& message)
{
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		logUserAction(sender->getNickname(), sender->getFd(),
			std::string("tried to send PRIVMSG to non-existing channel: ") + RED + channelName + RESET);
		sender->replyError(403, channelName, "No such channel");
		return;
	}
	if (!channel->is_user_member(sender->getNickname()))
	{
		logUserAction(sender->getNickname(), sender->getFd(), std::string("tried to send PRIVMSG to ")
			+ BLUE + channelName + RESET + " but is not a member");
		sender->replyError(404, channelName, "Cannot send to channel");
		return;
	}

	std::string	line = ":" + sender->buildPrefix() + " PRIVMSG " + channelName + " :" + message + "\r\n";
	Command::broadcastToChannel(server, channel, line, sender->getNickname());

	logUserAction(sender->getNickname(), sender->getFd(), std::string("sent PRIVMSG to ")
		+ BLUE + channelName + RESET);
}

/**
 Sends a PRIVMSG from a user to another user.

 @param server 		Pointer to the server instance.
 @param sender 		Pointer to the user sending the message.
 @param targetNick 	Nickname of the recipient user.
 @param message 	The message content to send.
*/
static void	handlePrivmsgToUser(Server* server, User* sender, const std::string& targetNick,
	const std::string& message)
{
	User*	targetUser = server->getUser(targetNick);
	if (!targetUser)
	{
		logUserAction(sender->getNickname(), sender->getFd(),
			std::string("tried to send PRIVMSG to non-existing user: ") + RED + targetNick + RESET);
		sender->replyError(401, targetNick, "No such nick/channel");
		return;
	}

	std::string	line = ":" + sender->buildPrefix() + " PRIVMSG " + targetNick + " :" + message;
	targetUser->getOutputBuffer() += line;

	logUserAction(sender->getNickname(), sender->getFd(),
		std::string("sent PRIVMSG to user ") + GREEN + targetNick + RESET);
}

/**
 Handles the PRIVMSG command from a user.
 Sends a message to one or more recipients, which can be users or channels.
 Format: `PRIVMSG <recipient>{,<recipient>} :<text to be sent>`

 @param server 	Pointer to the server instance.
 @param user 	Pointer to the user sending the message.
 @param tokens 	Tokenized input of the PRIVMSG command.
*/
void	Command::handlePrivmsg(Server *server, User *user, const std::vector<std::string> &tokens)
{
	if (!checkRegistered(user, "PRIVMSG"))
		return ;

	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid PRIVMSG command (too few arguments)");
		user->replyError(411, "", "No recipient given (PRIVMSG)");
		return;
	}
	else if (tokens.size() < 3)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid PRIVMSG command (too few arguments)");
		user->replyError(412, "", "No text to send");
		return;
	}

	// Split comma-separated list of targets
	std::vector<std::string>	targets = splitCommaList(tokens[1]);

	// Get message
	std::string	message = tokens[2];
	if (!message.empty() && message[0] == ':')
		message = message.substr(1); // Remove leading ':' if present

	// Send message to each target
	for (size_t i = 0; i < targets.size(); ++i)
	{
		const std::string&	target = targets[i];

		if (isValidChannelName(target)) // Channel
			handlePrivmsgToChannel(server, user, target, message);
		else // User
			handlePrivmsgToUser(server, user, target, message);
	}
}

////////////
// NOTICE //
////////////

/**
 Sends a NOTICE from a user to another user.

 @param server 		Pointer to the server instance.
 @param sender 		Pointer to the user sending the message.
 @param targetNick 	Nickname of the recipient user.
 @param message 	The message content to send.
*/
static void	handleNoticeToChannel(Server* server, User* sender, const std::string& channelName,
	const std::string& message)
{
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		logUserAction(sender->getNickname(), sender->getFd(),
			std::string("tried to send NOTICE to non-existing channel: ") + RED + channelName + RESET);
		return;
	}
	if (!channel->is_user_member(sender->getNickname()))
	{
		logUserAction(sender->getNickname(), sender->getFd(), std::string("tried to send NOTICE to ")
			+ BLUE + channelName + RESET + " but is not a member");
		return;
	}

	std::string	line = ":" + sender->buildPrefix() + " NOTICE " + channelName + " :" + message;
	Command::broadcastToChannel(server, channel, line, sender->getNickname());

	logUserAction(sender->getNickname(), sender->getFd(), std::string("sent NOTICE to ")
		+ BLUE + channelName + RESET);
}

/**
 Sends a NOTICE from a user to a channel.

 @param server 			Pointer to the server instance.
 @param sender 			Pointer to the user sending the message.
 @param channelName 	Name of the target channel.
 @param message 		The message content to send.
*/
static void	handleNoticeToUser(Server* server, User* sender, const std::string& targetNick,
	const std::string& message)
{
	User*	targetUser = server->getUser(targetNick);
	if (!targetUser)
	{
		logUserAction(sender->getNickname(), sender->getFd(),
			std::string("tried to send NOTICE to non-existing user: ") + RED + targetNick + RESET);
		return;
	}

	std::string	line = ":" + sender->buildPrefix() + " NOTICE " + targetNick + " :" + message;
	targetUser->getOutputBuffer() += line;

	logUserAction(sender->getNickname(), sender->getFd(),
		std::string("sent NOTICE to user ") + GREEN + targetNick + RESET);
}

/**
 Just like PRIVMSG, but does not send server replies to the user.

 Handles the NOTICE command from a user.
 Sends a message to one or more recipients, which can be users or channels.
 Format: `NOTICE <recipient>{,<recipient>} :<text to be sent>`

 @param server 	Pointer to the server instance.
 @param user 	Pointer to the user sending the message.
 @param tokens 	Tokenized input of the NOTICE command.
*/
void	Command::handleNotice(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (!checkRegistered(user, "NOTICE"))
		return ;

	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid NOTICE command (too few arguments)");
		return;
	}
	else if (tokens.size() < 3)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid NOTICE command (too few arguments)");
		return;
	}

	// Split comma-separated list of targets
	std::vector<std::string>	targets = splitCommaList(tokens[1]);

	// Get message
	std::string	message = tokens[2];
	if (!message.empty() && message[0] == ':')
		message = message.substr(1); // Remove leading ':' if present

	// Send message to each target
	for (size_t i = 0; i < targets.size(); ++i)
	{
		const std::string&	target = targets[i];

		if (isValidChannelName(target)) // Channel
			handleNoticeToChannel(server, user, target, message);
		else // User
			handleNoticeToUser(server, user, target, message);
	}
}
