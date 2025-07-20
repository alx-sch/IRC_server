#include <iostream>
#include <sstream>	// std::istringstream

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/defines.hpp"	// color definitions

/////////////////////
// Handle Commands //
///////////////////// 

bool	Command::handleCommand(Server* server, User* user, int fd, const std::string& message)
{
	if (message.empty())
		return false;

	std::vector<std::string>	tokens = Command::tokenize(message);
	if (tokens.empty())
		return false;

	Command::Type				cmdType = Command::getType(message);

	(void)server; // prob needed by other commands?
	(void)fd; // prob needed by other commands?

	switch (cmdType)
	{
		case NICK:		return handleNick(user, tokens);
		case USER:		return handleUser(user, tokens);
		case PASS:		return handlePass(server, user, tokens);
			// Handle PASS command
			break;
		case PING:
			// Handle PING command
			break;
		case JOIN:
			// Handle JOIN command
			break;
		case PART:
			// Handle PART command
			break;
		case QUIT:
			// Handle QUIT command
			break;
		case PRIVMSG:
			// Handle PRIVMSG command
			break;
		case TOPIC:
			// Handle TOPIC command
			break;
		case KICK:
			// Handle KICK command
			break;
		case INVITE:
			// Handle INVITE command
			break;
		case MODE:
			// Handle MODE command
			break;
		default:
			return false;
	}
	return true;
}

// Handles the  `NICK` command for a user. Also part of the initial client registration.
// Command:  `NICK <nickname> `
bool	Command::handleNick(User* user, const std::vector<std::string>& tokens)
{
	if (tokens.size() < 2)
	{
		std::cout	<< GREEN << user->getNickname() << RESET
					<< " (" << MAGENTA << "fd " << user->getFd() << RESET
					<< ") " << "sent NICK without a nickname\n";

		user->replyError(431, "", "No nickname given");
		return false;
	}

	const std::string&	nick = tokens[1];
	user->setNickname(nick);
	return true;
}

// Handles the  `USER` command for a user. Also part of the initial client registration.
// Command:  `USER <username> <hostname> <servername> :<realname> `
bool	Command::handleUser(User* user, const std::vector<std::string>& tokens)
{
	if (user->isRegistered())
	{
		std::cout	<< GREEN << user->getNickname() << RESET
					<< " (" << MAGENTA << "fd " << user->getFd() << RESET
					<< ") " << "tried to resend USER after registration\n";

		user->replyError(462, "", "You may not reregister");
		return false;
	}

	if (tokens.size() < 5)
		return false;

	user->setUsername(tokens[1]);
	user->setRealname(tokens[4]);
	return true;
}

// Handles the `PASS` command for a user. This is used to authenticate the user with the server.
// Command: `PASS <password>`
bool	Command::handlePass(Server* server, User* user, const std::vector<std::string>& tokens)
{
	// Reject if user already completed registration
	if (user->isRegistered())
	{
		std::cout	<< GREEN << user->getNickname() << RESET
					<< " (" << MAGENTA << "fd " << user->getFd() << RESET
					<< ") " << "tried to resend PASS after registration\n";

		user->replyError(462, "", "You may not reregister");
		return false; // Not sure if to return false here, might kick user from server?
	}

	// Reject if password argument is missing
	if (tokens.size() < 2)
		return false;

	// Validate the provided password
	// If server password is empty, any password is accepted
	const std::string&	password = tokens[1];
	if (server->getPassword() != "" && password != server->getPassword())
	{
		std::cout	<< GREEN << user->getNickname() << RESET
					<< " (" << MAGENTA << "fd " << user->getFd() << RESET
					<< ") " << "provided incorrect password\n";

		user->replyError(464, "", "Password incorrect");
		return false;
	}

	// Password check passed
	user->setHasPassed(true);
	user->tryRegister();
	return true;
}

///////////
// Utils //
///////////

/**
 Tokenizes a raw IRC message into space-separated parts,
 preserving the trailing parameter(after a colon `:`).
 
 This function splits an IRC line like:
	"USER max 0 * :Max Power the Third"
 into:
 	["USER", "max", "0", "*", "Max Power the Third"]

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
			std::string	trailing = token.substr(1); // strip leading ':' -> token: "Max"
			std::string	rest;
			std::getline(iss, rest); // read rest of the line -> rest: " Power the Third"
			tokens.push_back(trailing + rest); // Combine and add to tokens -> tokens: ["Max Power the Third"]
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
