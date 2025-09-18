#include <vector>
#include <string>
#include <algorithm>	// For std::transform

#include "../include/Command.hpp"
#include "../include/User.hpp"
#include "../include/utils.hpp"		// logUserAction()

static int	toUpperChar(int c);

// Extracts the command type from a message
// Returns `UNKNOWN` if no valid command is found
Command::Cmd	Command::getCmd(const std::vector<std::string>& tokens)
{
	if (tokens.empty())
		return UNKNOWN;

	std::string	cmd = tokens[0];

	// Make commands case-insensitive by converting to uppercase
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), toUpperChar);

	if (cmd == "NICK")		return NICK;
	if (cmd == "USER")		return USER;
	if (cmd == "PASS")		return PASS;
	if (cmd == "JOIN")		return JOIN;
	if (cmd == "QUIT")		return QUIT;
	if (cmd == "PART")		return PART;
	if (cmd == "PRIVMSG")	return PRIVMSG;
	if (cmd == "NOTICE")	return NOTICE;
	if (cmd == "TOPIC")		return TOPIC;
	if (cmd == "KICK")		return KICK;
	if (cmd == "INVITE")	return INVITE;
	if (cmd == "MODE")		return MODE;
	if (cmd == "LIST")		return LIST;

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
		user->sendError(451, "", "You have not registered");
		return false;
	}
	return true;
}

/**
Splits a comma-separated string into a vector of strings.

For example, given the input "#chan1,#chan2,#chan3",
it returns a vector containing {"#chan1", "#chan2", "#chan3"}.

 @param list	A string containing comma-separated tokens.
 @return		A vector of individual tokens split by commas.
*/
std::vector<std::string>	Command::splitCommaList(const std::string& list)
{
	std::vector<std::string>	result;
	size_t						start = 0;
	size_t						pos = 0;

	while (pos != std::string::npos)
	{
		pos = list.find(',', start);
		if (pos == std::string::npos)
		{
			result.push_back(list.substr(start)); // Last / only token
			break;
		}
		else
		{
			result.push_back(list.substr(start, pos - start));
			start = pos + 1; // Move past the comma
		}
	}
	return result;
}

////////////
// HELPER //
////////////


// Safely converts a character to its uppercase equivalent, as std::toupper
// may invoke undefined behavior when passed a negative char value.
// Used as unary operation in std::transform.
static int	toUpperChar(int c)
{
	return std::toupper(static_cast<unsigned char>(c));
}
