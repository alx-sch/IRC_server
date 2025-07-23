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
		case PING:
			// Handle PING command
			break;
		case JOIN:      return handleJoin(server, user, tokens);        
			break;
		case PART:
			// Handle PART command
			break;
		case QUIT:
			// Handle QUIT command
			break;
		case PRIVMSG:   return handlePrivmsg(server, user, tokens);
			break;
		case TOPIC:     return handleTopic(server, user, tokens);
			break;
		case KICK:
			// Handle KICK command
			break;
		case INVITE:    return handleInvite(server, user, tokens);
			break;
		case MODE:
			// Handle MODE command
			break;
		default:
			return false;
	}
	return true;
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
        std::cout << "Channel " << channelName << " created.\n";
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
    std::cout << GREEN << user->getNickname() << RESET
              << " (" << MAGENTA << "fd " << user->getFd() << RESET
              << ") " << "joined channel " << channelName << "\n";
    user->getInputBuffer() += "JOIN " + channelName + "\r\n";
    user->replyWelcome(); 
    return true;
    

}

bool Command::handlePrivmsg(Server *server, User *user, const std::vector<std::string> &tokens)
{
    if (tokens.size() < 3)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "sent PRIVMSG without enough arguments\n";
        user->replyError(411, "", "No recipient given");
        return false;
    }
    const std::string& targetName = tokens[1];
    if(targetName[0] == '#')
    {
          // It's a channel
            Channel *channel = server->getChannel(targetName);
            if (!channel)
            {
                std::cout << GREEN << user->getNickname() << RESET
                        << " (" << MAGENTA << "fd " << user->getFd() << RESET
                        << ") " << "tried to PRIVMSG non-existing channel: " << targetName << "\n";
                user->replyError(403, targetName, "No such nick/channel");
            }
            else if (!channel->is_user_member(user->getNickname()))
            {
                std::cout << GREEN << user->getNickname() << RESET
                        << " (" << MAGENTA << "fd " << user->getFd() << RESET
                        << ") " << "tried to PRIVMSG channel " << targetName
                        << " but is not a member\n";
                user->replyError(404, targetName, "Cannot send to channel (not a member)");
                return false;
            }
            // If we reach here, it's a valid channel and user is a member
            std::string message = tokens[2];

            std::cout << GREEN << user->getNickname() << RESET
                    << " (" << MAGENTA << "fd " << user->getFd() << RESET
                    << ") " << "sent PRIVMSG to channel " << targetName << "\n";
            return true;
    }
    else
        {
             // It's a user
    User *targetUser = server->getUser(targetName);
    if (!targetUser)
    {
        std::cout << GREEN << user->getNickname() << RESET
                  << " (" << MAGENTA << "fd " << user->getFd() << RESET
                  << ") " << "tried to PRIVMSG non-existing user: " << targetName << "\n";
        user->replyError(401, targetName, "No such nick/channel");
        return false;
    }
    // If we reach here, it's a valid user
    std::string message = tokens[2];
    targetUser->getInputBuffer() += "PRIVMSG " + targetName + " :" + message + "\r\n";
    std::cout << GREEN << user->getNickname() << RESET
              << " (" << MAGENTA << "fd " << user->getFd() << RESET
              << ") " << "sent PRIVMSG to user " << targetName << "\n";
    return true;
        }
}

bool handleTopic(Server *server, User *user, const std::vector<std::string> &tokens)
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
        user->getInputBuffer() += ":"
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
