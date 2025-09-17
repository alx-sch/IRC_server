#include <string>
#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidChannelName
#include "../include/defines.hpp"	// color formatting

static void	handleMessageToChannel(Server* server, User* sender, const std::string& channelName,
									const std::string& message, const std::string& commandName);
static void	handleMessageToUser(Server* server, User* sender, const std::string& targetNick,
								const std::string& message, const std::string& commandName);

/**
Handles sending a message (`PRIVMSG` or `NOTICE`) to users and channels.

 @param server		Pointer to the server instance.
 @param user		Pointer to the user sending the message.
 @param tokens		Tokenized input of the command.
 @param commandName	The name of the command being executed ("PRIVMSG" or "NOTICE").
*/
void	Command::handleMessage(Server* server, User* user, const std::vector<std::string>& tokens,
								const std::string& commandName)
{
	// Determine if the command should send error replies.
	const bool	sendReplies = (commandName == "PRIVMSG");

	if (!checkRegistered(user, commandName))
		return;

	// Argument checks
	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid " + commandName + " (no recipient)");
		if (sendReplies)
			user->replyError(411, "", "No recipient given (" + commandName + ")");
		return;
	}
	if (tokens.size() < 3)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid " + commandName + " (no text)");
		if (sendReplies)
			user->replyError(412, "", "No text to send");
		return;
	}

	// Get message
	std::string	message = tokens[2];
	if (!message.empty() && message[0] == ':')
		message = message.substr(1); // Remove leading ':' if present

	// Send message to each target
	std::vector<std::string>	targets = splitCommaList(tokens[1]);
	for (size_t i = 0; i < targets.size(); ++i)
	{
		const std::string&	target = targets[i];
		if (isValidChannelName(target)) // Channel
			handleMessageToChannel(server, user, target, message, commandName);
		else // User
			handleMessageToUser(server, user, target, message, commandName);
	}
}

/**
Handles the `PRIVMSG` command from a user.
Sends a message to one or more recipients, which can be users or channels.
Format: `PRIVMSG <recipient>{,<recipient>} :<text to be sent>`

 @param server	Pointer to the server instance.
 @param user	Pointer to the user sending the message.
 @param tokens	Tokenized input of the PRIVMSG command.
*/
void Command::handlePrivmsg(Server *server, User *user, const std::vector<std::string> &tokens)
{
	handleMessage(server, user, tokens, "PRIVMSG");
}

/**
Handles the `NOTICE` command from a user.
Like `PRIVMSG`, but does not trigger automatic replies from the server.

Sends a message to one or more recipients, which can be users or channels.
Format: `NOTICE <recipient>{,<recipient>} :<text to be sent>`

 @param server	Pointer to the server instance.
 @param user	Pointer to the user sending the message.
 @param tokens	Tokenized input of the NOTICE command.
*/
void Command::handleNotice(Server *server, User *user, const std::vector<std::string> &tokens)
{
	handleMessage(server, user, tokens, "NOTICE");
}

////////////
// HELPER //
////////////

/**
Sends a message (`PRIVMSG` or `NOTICE`) from a user to a channel.

 @param commandName	The name of the command ("PRIVMSG" or "NOTICE").
*/
static void	handleMessageToChannel(Server* server, User* sender, const std::string& channelName,
									const std::string& message, const std::string& commandName)
{
	const bool	sendReplies = (commandName == "PRIVMSG");
	Channel*	channel = server->getChannel(channelName);

	if (!channel)
	{
		logUserAction(sender->getNickname(), sender->getFd(), "tried to send " + commandName
			+ " to non-existing " + RED + channelName + RESET);
		if (sendReplies)
			sender->replyError(403, channelName, "No such channel");
		return;
	}
	if (!channel->is_user_member(sender))
	{
		logUserAction(sender->getNickname(), sender->getFd(), "tried to send " + commandName
			+ " to " + BLUE + channelName + RESET + " but is not a member");
		if (sendReplies)
			sender->replyError(404, channelName, "Cannot send to channel");
		return;
	}

	// Construct the IRC line and broadcast it
	std::string line = ":" + sender->buildHostmask() + " " + commandName + " " + channelName + " :" + message;
	Command::broadcastToChannel(channel, line, sender->getNicknameLower()); // exclude sender

	logUserAction(sender->getNickname(), sender->getFd(), "sent " + commandName + " to "
		+ BLUE + channelName + RESET);
}

/**
Sends a message (`PRIVMSG` or `NOTICE`) from a user to another user.

 @param commandName	The name of the command ("PRIVMSG" or "NOTICE").
*/
static void handleMessageToUser(Server* server, User* sender, const std::string& targetNick,
								const std::string& message, const std::string& commandName)
{
	const bool	sendReplies = (commandName == "PRIVMSG");
	User*		targetUser = server->getUser(normalize(targetNick));

	if (!targetUser)
	{
		logUserAction(sender->getNickname(), sender->getFd(), "tried to send " + commandName
			+ " to non-existing " + RED + targetNick + RESET);
		if (sendReplies)
			sender->replyError(401, targetNick, "No such nick/channel");
		return;
	}

	// Construct the IRC line and add to the target user's output buffer
	std::string	line =	":" + sender->buildHostmask() + " " + commandName + " " + targetUser->getNickname()
							+ " :" + message + "\r\n";
	targetUser->getOutputBuffer() += line;

	logUserAction(sender->getNickname(), sender->getFd(), "sent " + commandName + " to user "
		+ GREEN + targetUser->getNickname() + RESET);
}
