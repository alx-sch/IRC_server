#include <string>
#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// isValidChannelName()
#include "../include/defines.hpp"	// color formatting

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
		user->logUserAction("sent invalid " + commandName + " (no recipient)");
		if (sendReplies)
			user->sendError(411, "", "No recipient given (" + commandName + ")");
		return;
	}
	if (tokens.size() < 3)
	{
		user->logUserAction("sent invalid " + commandName + " (no text)");
		if (sendReplies)
			user->sendError(412, "", "No text to send");
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
void	Command::handleMessageToChannel(Server* server, User* sender, const std::string& channelName,
									const std::string& message, const std::string& commandName)
{
	const bool	sendReplies = (commandName == "PRIVMSG");
	Channel*	channel = server->getChannel(channelName);

	if (!channel)
	{
		sender->logUserAction("tried to send " + commandName
			+ " to non-existing " + RED + channelName + RESET);
		
		if (sendReplies)
			sender->sendError(403, channelName, "No such channel");
		return;
	}

	std::string	channelNameOrig = channel->get_name();
	if (!channel->is_user_member(sender))
	{
		sender->logUserAction("tried to send " + commandName
			+ " to " + BLUE + channelNameOrig + RESET + " but is not a member");
		// Only send error for PRIVMSG, NOTICE does not trigger replies
		if (sendReplies)
			sender->sendError(404, channelNameOrig, "Cannot send to channel");
		return;
	}

	// Construct the IRC line and broadcast it
	std::string	line = ":" + sender->buildHostmask() + " " + commandName + " " + channelNameOrig + " :" + message;
	Command::broadcastToChannel(channel, line, sender->getNicknameLower()); // exclude sender

	sender->logUserAction("sent " + commandName + " to " + BLUE + channelNameOrig + RESET);
}

/**
Sends a message (`PRIVMSG` or `NOTICE`) from a user to another user.

 @param server		Pointer to the server instance.
 @param sender		Pointer to the user sending the message.
 @param targetNick	The nickname of the target user.
 @param message		The message text to send.
 @param commandName	The name of the command ("PRIVMSG" or "NOTICE").
 @param botCmd		Optional, if the sender is a bot (specifies command the bot is replying to).
*/
void Command::handleMessageToUser(Server* server, User* sender, const std::string& targetNick,
								const std::string& message, const std::string& commandName, const std::string& botCmd)
{
	const bool	sendReplies = (commandName == "PRIVMSG");
	User*		targetUser = server->getUser(normalize(targetNick));

	if (BOT_SILENT_NOTE && commandName == "NOTICE" && sender->getIsBot())
		return; // Do not log bot NOTICE messages

	std::string	logCmd = commandName;
	if (sender->getIsBot() && !botCmd.empty())
		logCmd += " (" + botCmd + ")";

	if (!targetUser)
	{
		sender->logUserAction("tried to send " + logCmd
			+ " to non-existing " + RED + targetNick + RESET);
		if (sendReplies)
			sender->sendError(401, targetNick, "No such nick/channel");
		return;
	}

	// Construct the IRC line and add to the target user's output buffer
	std::string	line =	commandName + " " + targetUser->getNickname() + " :" + message;
	targetUser->sendMsgFromUser(sender, line);

	sender->logUserAction("sent " + logCmd + " to user "
		+ GREEN + targetUser->getNickname() + RESET, sender->getIsBot());
}
