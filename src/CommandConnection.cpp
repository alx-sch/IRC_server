#include <set>
#include <string>
#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction
#include "../include/defines.hpp"	// color formatting

// Helper function to get the quit reason from the tokens
static std::string	getQuitReason(const std::vector<std::string>& tokens)
{
	if (tokens.size() < 2 || tokens[1].empty())
		return "Client Quit";	// Default reason if none provided

	std::string reason = tokens[1];
	if (reason[0] == ':')	// Remove leading ':' if present
		reason = reason.substr(1);
	return reason;
}

/**
Handles the `QUIT` command from a user.

This function broadcasts the quit message to all channels the user is in,
removes the user from those channels, and deletes the user from the server.

 @param server		Pointer to the Server object handling the connection.
 @param user		Pointer to the User who issued the QUIT command.
 @param tokens		Vector of parsed command tokens. `tokens[1]` can contain an optional reason.
*/
void	Command::handleQuit(Server* server, User* user, const std::vector<std::string>& tokens)
{
	std::string	reason = getQuitReason(tokens);
	std::string	userNick = user->getNickname();
	
	// Get the list of channels the user is in
	const std::set<std::string>&	channels = user->getChannels();

	// Iterate through each channel the user is in
	for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		Channel*	channel = server->getChannel(*it); // Get the channel object by name
		if (!channel)
			continue; // Skip if channel does not exist

		// Broadcast the quit message to all channel members
		std::string	quitMsg = ":" + user->buildHostmask() + " QUIT :" + reason;
		broadcastToChannel(server, channel, quitMsg, userNick);

		// Remove the user from the channel
		channel->remove_user(userNick);
		logUserAction(userNick, user->getFd(), toString("left ") + BLUE + *it + RESET
			+ ": " + YELLOW + reason + RESET);
	}
	server->deleteUser(user->getFd(), toString("quit: ") + YELLOW + reason + RESET);	// Remove user from server
}
