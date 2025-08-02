#include <iostream>
#include <sstream>	// std::istringstream

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
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
 @param message		The raw IRC message line received from the user/client.

 @return 			`true` if the command was successfully recognized and handled;
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
			// Handle PASS command
			break;
		case PING:      return handlePing(server, user, tokens);
			// Handle PING command
			break;
		case JOIN:      return handleJoin(server, user, tokens);        
			break;
		case PART:      return handlePart(server, user, tokens);
			// Handle PART command
			break;
		case QUIT:      return handleQuit(server, user, tokens);
			// Handle QUIT command
			break;
		case PRIVMSG:   return handlePrivmsg(server, user, tokens);
			break;
		case TOPIC:     return handleTopic(server, user, tokens);
			break;
		case KICK:      return handleKick(server, user, tokens);
			// Handle KICK command
			break;
		case INVITE:    return handleInvite(server, user, tokens);
			break;
		case MODE:       return handleMode(server, user, tokens);
			// Handle MODE command
			break;
		default:
			return false;
	}
	return true;
}

bool Command::handleQuit(Server* server, User* user, const std::vector<std::string>& tokens)
{
    if (tokens.size() > 2)
    {
        //for each channel the user is in, send a quit message to the channel
        //prepended with : Quit :
        for (std::set<std::string>::const_iterator it = user->getChannels().begin();
             it != user->getChannels().end(); ++it)
        {
            Channel* channel = server->getChannel(*it);
            std::string message = ":" + user->getNickname() + " QUIT :" + tokens[1] + "\r\n";
            sndMsgAllInChannel(server, user, *it, message, channel);
        }
        //send quit message to all channels prepended with : Quit :
    }
    // the user is quitting, so we need to remove them from all channels
    // remove them from all channels
    // remove them from the server's user map
    // remove them from the server's nick map
    // remove them from the server's user list
    return false; // TODO: Implement complete QUIT functionality
}

bool Command::handlePing(Server*, User*, const std::vector<std::string>&)
{
  /*
  The PING command is sent by either clients or servers to check the other side
of the connection is still connected and/or to check for connection latency, at
the application layer.

The <token> may be any non-empty string.

When receiving a PING message, clients or servers must reply to it with a PONG
message with the same <token> value. This allows either to match PONG with the
PING they reply to, for example to compute latency.

Clients should not send PING during connection registration, though servers may
accept it. Servers may send PING during connection registration and clients must
reply to them.
  */
  return false; // TODO: Implement PING/PONG functionality
}

bool Command::handlePart(Server*, User*, const std::vector<std::string>&)
{
    /*
    The PART command is used to leave a channel.
    */
    return false; // TODO: Implement PART functionality
}

bool Command::handleKick(Server*, User*, const std::vector<std::string>&)
{
    /*
    The KICK command is used to remove a user from a channel.
    */
    return false; // TODO: Implement KICK functionality
}

bool Command::handleMode(Server*, User*, const std::vector<std::string>&)
{
    /*
    The MODE command is used to set or change a channel's mode.
    */
    return false; // TODO: Implement MODE functionality
}

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
	
	user->setUsername(tokens[1]);
	user->setHost(tokens[2]);
	user->setRealname(tokens[4].substr(1)); // Remove leading colon from realname

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

	user->setHasPassed(true);
	user->tryRegister();
	return true;
}

bool     Command::handleJoin(Server* server, User* user, const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "sent JOIN without a channel name\n";
        user->replyError(403, "", "No channel specified");
        return false;
    }
    const std::string& channelName = tokens[1];
    if (channelName.empty() || channelName[0] != '#')
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "sent JOIN with invalid channel name: " << channelName << "\n";
        user->replyError(403, channelName, "Invalid channel name");
        return false;
    }
    Channel* channel = server->getChannel(channelName);
    if (!channel)
    {
        channel = new Channel( channelName);
        if (!channel)
        {
            std::cerr << "Failed to create channel: " << channelName << std::endl;
            user->replyError(500, "", "Internal server error");
            return false;
        }
        server->addChannel(channel);
        logServerMessage("Channel " + channelName + " created");
    }
    if (tokens.size() > 2 && tokens[2][0] == ':')
    {
        std::string providedKey = tokens[2].substr(1); 
        if (!channel->validate_password(providedKey))
        {
            std::cout << GREEN << user->getNickname() << RESET
                      << " (" << MAGENTA << "fd " << user->getFd() << RESET
                      << ") " << "tried to join channel " << channelName
                      << " with incorrect password\n";
            user->replyError(475, channelName, "Cannot join channel (bad password)");
            return false;
        }
    }
    else if (!channel->can_user_join(user->getNickname(), ""))
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "tried to join channel " << channelName << " but cannot join\n";
        if (channel->is_invite_only() && !channel->is_invited(user->getNickname()))
            user->replyError(473, channelName, "Cannot join channel (invite only)");
        else if (channel->has_user_limit() && channel->is_at_user_limit())
            user->replyError(471, channelName, "Cannot join channel (user limit reached)");
        return false;
    }
    channel->add_user(user->getNickname());
    user->addChannel(channelName);
    std::cout << GREEN << user->getNickname() << RESET
              << " (" << MAGENTA << "fd " << user->getFd() << RESET
              << ") " << "joined channel " << channelName << "\n";
    // Send JOIN confirmation back to the user
    user->getOutputBuffer() += ":" + user->getNickname() + " JOIN " + channelName + "\r\n";
    user->replyWelcome(); 
    return true;
    

}

void Command::sndMsgAllInChannel(Server *&server, User *&user,
                        const std::string &targetName, std::string &message,
                        Channel *&channel) {
  const std::set<std::string> &members = channel->get_members();
  std::string privmsgLine = ":" + user->getNickname() + " PRIVMSG " +
                            targetName + " :" + message + "\r\n";

  for (std::set<std::string>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    if (*it != user->getNickname()) { // Don't send to sender
      User *member = server->getUser(*it);
      if (member) {
        member->getOutputBuffer() += privmsgLine;
      }
    }
  }
}
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
    sndMsgAllInChannel(server, user, targetName, message, channel);

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

bool Command::handleTopic(Server *server, User *user, const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "sent TOPIC without a channel name\n";
        user->replyError(403, "", "No channel specified");
        return false;
    }
    const std::string& channelName = tokens[1];
    Channel* channel = server->getChannel(channelName);
    if (!channel)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "tried to set topic for non-existing channel: " << channelName << "\n";
        user->replyError(403, channelName, "No such channel");
        return false;
    }
    if (!channel->is_user_member(user->getNickname()))
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "tried to set topic for channel "
                  << channelName << ", but is not a member\n";
        user->replyError(442, channelName, "You're not on that channel");
        return false;
    }
    if (tokens.size() > 2)
    {
        std::string newTopic = tokens[2];
        if (newTopic[0] == ':')
            newTopic = newTopic.substr(1); // Remove leading ':'
        if (channel->has_topic_protection() && !channel->is_user_operator(user->getNickname()))
        {
            std::cout << GREEN << user->getNickname() << RESET
                      << " (" << MAGENTA << "fd " << user->getFd() << RESET
                      << ") " << "tried to set topic for channel "
                      << channelName << ", but is not an operator\n";
            user->replyError(482, channelName, "You're not channel operator");
            return false;
        }
        channel->set_topic(newTopic);
        
        // Broadcast topic change to all channel members
        const std::set<std::string>& members = channel->get_members();
        std::string topicLine = ":" + user->getNickname() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
        
        for (std::set<std::string>::const_iterator it = members.begin(); it != members.end(); ++it) {
            User* member = server->getUser(*it);
            if (member) {
                member->getOutputBuffer() += topicLine;
            }
        }
        
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") set topic for channel "
                  << channelName
                  << ": \"" + newTopic + "\"\n";
        return true;
    }
    else
    {
        // No new topic provided, just show current topic
        std::string currentTopic = channel->get_topic();
        if (currentTopic.empty())
            currentTopic = "(no topic)";
        user->getOutputBuffer() += ":"
                                  + server->getServerName() + " TOPIC "
                                  + channelName + " :" + currentTopic + "\r\n";
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") requested topic for channel "
                  << channelName << ": \"" + currentTopic + "\"\n";
        return true;
    }
}

bool Command::handleInvite(Server* server, User* user, const std::vector<std::string>& tokens)
{
    if (tokens.size() < 3)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "sent INVITE without enough arguments\n";
        user->replyError(461, "", "Not enough parameters");
        return false;
    }
    
    const std::string& targetNick = tokens[1];
    const std::string& channelName = tokens[2];
    
    // Check if channel exists
    Channel* channel = server->getChannel(channelName);
    if (!channel)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") tried to invite to non-existing channel: " << channelName << "\n";
        user->replyError(403, channelName, "No such channel");
        return false;
    }
    
    // Check if user is on the channel (required to invite others)
    if (!channel->is_user_member(user->getNickname()))
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") tried to invite to channel " << channelName
                  << " but is not a member\n";
        user->replyError(442, channelName, "You're not on that channel");
        return false;
    }
    
    // For invite-only channels, check if user is operator
    if (channel->is_invite_only() && !channel->is_user_operator(user->getNickname()))
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") tried to invite to invite-only channel "
                  << channelName << " but is not an operator\n";
        user->replyError(482, channelName, "You're not channel operator");
        return false;
    }
    
    // Check if target user exists
    User* targetUser = server->getUser(targetNick);
    if (!targetUser)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") tried to invite non-existing user: " << targetNick << "\n";
        user->replyError(401, targetNick, "No such nick/channel");
        return false;
    }
    
    // Check if target is already on the channel
    if (channel->is_user_member(targetNick))
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") tried to invite already member " << targetNick
                  << " to channel " << channelName << "\n";
        user->replyError(443, targetNick + " " + channelName, "is already on channel");
        return false;
    }
    
    // Add to invite list (for invite-only channels)
    if (channel->is_invite_only())
    {
        channel->add_invite(targetNick);
    }
    
// here we need to send the invite message to the target user
// and also a reply success number
    std::cout << GREEN << user->getNickname() << RESET
              << " (" << MAGENTA << "fd " << user->getFd() << RESET
              << ") invited " << targetNick << " to channel " << channelName << "\n";
    
    return true;
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
	if (cmd == "PING")		return PING;
	if (cmd == "JOIN")		return JOIN;
	if (cmd == "PART")		return PART;
	if (cmd == "QUIT")		return QUIT;
	if (cmd == "PRIVMSG")	return PRIVMSG;
	if (cmd == "TOPIC")		return TOPIC;
	if (cmd == "KICK")		return KICK;
	if (cmd == "INVITE")	return INVITE;
	if (cmd == "MODE")		return MODE;

	return UNKNOWN;
}
