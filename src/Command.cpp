#include <map>
#include <string>	// std::string::size_type (size / position in strings)

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"

/**
Handles a single IRC command received from a client.

This function tokenizes the raw input `message`, determines the command,
and calls the appropriate handler (e.g. handleNick, handleUser, etc.).

 @param server	Pointer to the IRC server instance.
 @param user	Pointer to the User object.
 @param message	The raw IRC message line received from the user/client.

 @return		`true` if the command was successfully recognized and handled;
				`false` if the message was empty, cmd was unknown, or cmd exec failed.
*/
bool	Command::handleCommand(Server* server, User* user, std::vector<std::string>& tokens)
{
	if (tokens.empty())
		return false;

	Cmd	cmdType = getCmd(tokens);

	switch (cmdType)
	{
		case NICK:		handleNick(server, user, tokens); break;
		case USER:		handleUser(user, tokens); break;
		case PASS:		handlePass(server, user, tokens); break;
		case JOIN:		handleJoin(server, user, tokens); break;
		case QUIT:		handleQuit(server, user, tokens); break;
		case PART:		handlePart(server, user, tokens); break;
		case PRIVMSG:	handlePrivmsg(server, user, tokens); break;
		case NOTICE:	handleNotice(server, user, tokens); break;
		case TOPIC:		handleTopic(server, user, tokens); break;
		case KICK:		handleKick(server, user, tokens); break;
		case INVITE:	handleInvite(server, user, tokens); break;
		case MODE:		handleMode(server, user, tokens); break;
		case LIST:		handleList(server, user); break;
		// case WHO:		handleWho(server, user, tokens); break;
		default:
			return false;	// unknown command
	}
	return true;
}

/**
Sends a message to all members of a given channel, optionally excluding one user.

 @param channel		Pointer to the channel whose members will receive the message.
 @param message		The message to broadcast (without trailing "\r\n")
 @param excludeNick	Optional nickname of a user to exclude from receiving the message.
*/
void	Command::broadcastToChannel(Channel* channel, const std::string& message,const std::string& excludeNick)
{
	const std::map<std::string, User*>&	members = channel->get_members();
	std::string							formattedMessage = message + "\r\n";

	for (std::map<std::string, User*>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		// Skip excluded user if specified
		if (!excludeNick.empty() && it->first == excludeNick)
			continue;

		User*	member = it->second;
		if (member)
			member->getOutputBuffer() += formattedMessage;
	}
}

/**
Tokenizes a raw IRC message into space-separated parts,
preserving the trailing parameter (after a colon `:`).
 
This function splits an IRC line like:
	"   USER max 0   * :Max Power  the Third"
 into:
	["USER", "max", "0", "*", ":Max Power  the Third"]

If a token starts with a colon (`:`), the rest of the line (including spaces) is treated
as a single argument (the trailing parameter), as per IRC protocol.

 @param message	The raw IRC message line.
 @return		A vector of tokens: command + arguments (with trailing combined).
*/
std::vector<std::string>	Command::tokenize(const std::string& message)
{
	std::vector<std::string>	tokens;
	std::string::size_type		pos = 0;	// size_type is for string positions
	std::string::size_type		end;

	while (pos < message.size())
	{
		// Skip leading spaces
		while (pos < message.size() && message[pos] == ' ')
			++pos;

		if (pos >= message.size())
			break; // No more tokens

		// If token starts with ':', rest is trailing param
		if (message[pos] == ':')
		{
			if (pos + 1 < message.size()) // Only push trailing param if there is something after ':'
				tokens.push_back(message.substr(pos)); // Add the rest of the line as one token
			break; // No more tokens
		}

		// Find next space (after token) to extract the token
		end = message.find(' ', pos);
		if (end == std::string::npos)
			end = message.size(); // No more spaces, take the rest of the line

		tokens.push_back(message.substr(pos, end - pos));
		pos = end + 1; // Move past the space
	}
	return tokens;
}
