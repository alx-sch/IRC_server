#include <iostream>
#include <sstream>	// std::istringstream

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidNick
#include "../include/defines.hpp"	// color formatting

/////////////////////
// Handle Commands //
/////////////////////

/**
 Handles a single IRC command received from a client.

 This function tokenizes the raw input `message`, determines the command type,
 and calls the appropriate handler (e.g. handleNick, handleUser, etc.).

 @param server		Pointer to the IRC server instance.
 @param user		Pointer to the User object.
 @param message	The raw IRC message line received from the user/client.

 @return				`true` if the command was successfully recognized and handled;
						`false` if the message was empty, command was unknown, or command execution failed.
*/
bool	Command::handleCommand(Server* server, User* user, const std::string& message)
{
	if (message.empty())
		return false;

	std::vector<std::string>	tokens = Command::tokenize(message);
	if (tokens.empty())
		return false;

	Command::Type				cmdType = Command::getType(message);

	switch (cmdType)
	{
		case NICK:		return handleNick(server, user, tokens);
		case USER:		return handleUser(user, tokens);
		case PASS:		return handlePass(server, user, tokens);
		case JOIN:		return handleJoin(server, user, tokens);
		case PART:		return handlePart(server, user, tokens);
		case PRIVMSG:	return handlePrivmsg(server, user, tokens);
		case NOTICE:	return handleNotice(server, user, tokens);
		case TOPIC:		return handleTopic(server, user, tokens);
		case KICK:		return handleKick(server, user, tokens);
		case INVITE:	return handleInvite(server, user, tokens);
		case MODE:		return handleMode(server, user, tokens);
		default:
			return false;
	}
	return true;
}

////////////////////////
// MESSAGING COMMANDS //
////////////////////////

bool Command::handlePrivmsg(Server *server, User *user,
                            const std::vector<std::string> &tokens) {
  if (tokens.size() < 3) {
    std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
              << "fd " << user->getFd() << RESET << ") "
              << "sent PRIVMSG without enough arguments\n";
    user->replyError(411, "", "No recipient given");
    return false;
  }

  const std::string &targetName = tokens[1];

  // Reconstruct the full message from tokens[2] onward
  std::string message;
  for (size_t i = 2; i < tokens.size(); ++i) {
    if (i > 2)
      message += " ";
    message += tokens[i];
  }

  // Remove leading ':' if present (trailing parameter)
  if (!message.empty() && message[0] == ':') {
    message = message.substr(1);
  }

  if (targetName[0] == '#') {
    // It's a channel
    Channel *channel = server->getChannel(targetName);
    if (!channel) {
      std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
                << "fd " << user->getFd() << RESET << ") "
                << "tried to PRIVMSG non-existing channel: " << targetName
                << "\n";
      user->replyError(403, targetName, "No such nick/channel");
      return false;
    }

    if (!channel->is_user_member(user->getNickname())) {
      std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
                << "fd " << user->getFd() << RESET << ") "
                << "tried to PRIVMSG channel " << targetName
                << " but is not a member\n";
      user->replyError(404, targetName,
                       "Cannot send to channel (not a member)");
      return false;
    }

    // Broadcast message to all channel members except sender
    std::string privmsgLine = ":" + user->getNickname() + " PRIVMSG " + targetName + " :" + message + "\r\n";
    broadcastToChannel(server, channel, privmsgLine, user->getNickname());

    std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
              << "fd " << user->getFd() << RESET << ") "
              << "sent PRIVMSG to channel " << targetName << ": " << message
              << "\n";
    return true;
  } else {
    // It's a user
    User *targetUser = server->getUser(targetName);
    if (!targetUser) {
      std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
                << "fd " << user->getFd() << RESET << ") "
                << "tried to PRIVMSG non-existing user: " << targetName << "\n";
      user->replyError(401, targetName, "No such nick/channel");
      return false;
    }

    // Send private message to target user
    std::string privmsgLine = ":" + user->getNickname() + " PRIVMSG " +
                              targetName + " :" + message + "\r\n";
    targetUser->getOutputBuffer() += privmsgLine;

    std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
              << "fd " << user->getFd() << RESET << ") "
              << "sent PRIVMSG to user " << targetName << ": " << message
              << "\n";
    return true;
  }
}

bool	Command::handleNotice(Server* server, User* user, const std::vector<std::string>& tokens)
{
	(void)server;
	(void)user;
	(void)tokens;

	return true; // To be implemented
}

///////////
// Utils //
///////////

/**
 Tokenizes a raw IRC message into space-separated parts,
 preserving the trailing parameter (after a colon `:`).
 
 This function splits an IRC line like:
	"USER max 0 * :Max Power the Third"
 into:
 	["USER", "max", "0", "*", ":Max Power the Third"]

 If a token starts with a colon (`:`), the rest of the line (including spaces) is treated
 as a single argument (the trailing parameter), as per IRC protocol.

 @param message 	The raw IRC message line.
 @return 			A vector of tokens: command + arguments (with trailing combined).
*/
std::vector<std::string>	Command::tokenize(const std::string& message)
{
	std::string					token;
	std::vector<std::string>	tokens;
	std::istringstream			iss(message);	// Convert string to stream for easy tokenization

	// Extract tokens by whitespace ('iss >>' skips repeated spaces)
	while (iss >> token)
	{
		// If token starts with ':', treat rest of line as one token
		// Assume "USER max 0 * :Max Power the Third"
		if (token[0] == ':') // token: ":Max"
		{
			std::string	rest;
			std::getline(iss, rest); // read rest of the line -> rest: " Power the Third"
			tokens.push_back(token + rest); // Combine and add to tokens -> tokens: [":Max Power the Third"]
			break; // After processing ':', stop reading more tokens (':' indicates last token)
		}
		else
			tokens.push_back(token);
	}
	return tokens;
}

// Extracts the command type from a message
// Returns `UNKNOWN` if no valid command is found
Command::Type Command::getType(const std::string& message)
{
	std::vector<std::string>	tokens = tokenize(message);
	if (tokens.empty())
		return UNKNOWN;
	const std::string&			cmd = tokens[0];

	if (cmd == "NICK")		return NICK;
	if (cmd == "USER")		return USER;
	if (cmd == "PASS")		return PASS;
	if (cmd == "JOIN")		return JOIN;
	if (cmd == "PART")		return PART;
	if (cmd == "PRIVMSG")	return PRIVMSG;
	if (cmd == "NOTICE")	return NOTICE;
	if (cmd == "TOPIC")		return TOPIC;
	if (cmd == "KICK")		return KICK;
	if (cmd == "INVITE")	return INVITE;
	if (cmd == "MODE")		return MODE;

	return UNKNOWN;
}

// Checks if the user is registered before executing a command.
// If not, sends an error reply and returns false
bool	Command::checkRegistered(User* user, const std::string& command)
{
	if (!user->isRegistered())
	{
		logUserAction(user->getNickname(), user->getFd(),
			"tried to execute " + command + " before registration");
		user->replyError(451, "", "You have not registered");
		return false;
	}
	return true;
}

/**
 Sends a message to all members of a given channel, optionally excluding one user.

 @param server 		Pointer to the server object for user lookup.
 @param channel		Pointer to the channel whose members will receive the message.
 @param message		The message to broadcast (without trailing "\r\n")
 @param excludeNick	Optional nickname of a user to exclude from receiving the message.
*/
void	Command::broadcastToChannel(Server* server, Channel* channel, const std::string& message,
									const std::string& excludeNick)
{
	const std::set<std::string>&	members = channel->get_members();
	std::string						formattedMessage = message + "\r\n";

	for (std::set<std::string>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		// Skip excluded user if specified
		if (!excludeNick.empty() && *it == excludeNick)
			continue;

		User*	member = server->getUser(*it);
		if (member)
			member->getOutputBuffer() += formattedMessage;
	}
}
