#include <string>
#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"
#include "../include/defines.hpp" // color formatting

// XXX
static void	handleMsgToChannel(Server* server, User* sender, const std::string& channelName,
	const std::string& message)
{
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		logUserAction(sender->getNickname(), sender->getFd(),
			std::string("tried to send message to non-existing channel: ") + RED + channelName + RESET);
		sender->replyError(403, channelName, "No such channel");
		return;
	}
	if (!channel->is_user_member(sender->getNickname()))
	{
		logUserAction(sender->getNickname(), sender->getFd(), std::string("tried to send message to ")
			+ BLUE + channelName + RESET + " but is not a member");
		sender->replyError(404, channelName, "Cannot send to channel");
		return;
	}

	std::string	line = ":" + sender->buildPrefix() + " PRIVMSG " + channelName + " :" + message;
	Command::broadcastToChannel(server, channel, line, sender->getNickname());

	logUserAction(sender->getNickname(), sender->getFd(), std::string("sent PRIVMSG to ")
		+ BLUE + channelName + RESET);
}

static void	handleMsgToUser(Server* server, User* sender, const std::string& targetNick,
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

		if (target.empty())
			continue;

		if (isValidChannelName(target)) // Channel
			handleMsgToChannel(server, user, target, message);
		else // User
			handleMsgToUser(server, user, target, message);
	}
}

void	Command::handleNotice(Server* server, User* user, const std::vector<std::string>& tokens)
{
	(void)server;
	(void)user;
	(void)tokens;
}