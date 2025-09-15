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
	std::string	quitMsg = ":" + user->buildHostmask() + " QUIT :" + reason;

	// Build unique set of users to notify (to avoid duplicate messages)
	std::set<User*>			recipients;
	const std::set<std::string>&	channels = user->getChannels();

	// Iterate through each channel the user is in
	for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		Channel*	channel = server->getChannel(*it); // Get the channel object by name
		if (!channel)
			continue; // Skip if channel does not exist
		
		// Get all members of the channel and add them to recipients set (unique)
		const std::set<std::string>&	members = channel->get_members();
		for (std::set<std::string>::const_iterator mem_it = members.begin(); mem_it != members.end(); ++mem_it)
		{
			User*	member = server->getUser(*mem_it);
			if (member)
				recipients.insert(member);
		}
	}

	// Quitting message should not be sent to the quitting user
	recipients.erase(user);

	// Broadcast the quit message to all user which shared a channel with the quitting user
	for (std::set<User*>::const_iterator it = recipients.begin(); it != recipients.end(); ++it)
		(*it)->getOutputBuffer() += quitMsg + "\r\n";

	// Now remove the user from all channels they were in
	for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		Channel*	channel = server->getChannel(*it);
		if (channel)
			channel->remove_user(userNick);

		logUserAction(userNick, user->getFd(), toString("left ") + BLUE + *it + RESET
			+ ": " + YELLOW + reason + RESET);
	}
	// Finally, remove the user from the server
	server->deleteUser(user->getFd(), toString("quit: ") + YELLOW + reason + RESET);	// Remove user from server
}
