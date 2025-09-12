#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidChannelName
#include "../include/defines.hpp"	// color formatting

/**
Handles a single `JOIN` command for a single channel/key pair.

Validates channel name, checks if user is already a member, and verifies
channel restrictions (invite-only, user limit, password).
On success, adds the user to the channel and broadcasts the join message.

 @param server		Pointer to the server instance.
 @param user		Pointer to the user joining the channel.
 @param channelName	Name of the channel to join.
 @param key			Optional channel key (password) provided by the user.

 @return			True if the join succeeded,
					false if an error occurred.
*/
bool	Command::handleSingleJoin(Server* server, User* user, const std::string& channelName, const std::string& key)
{
	// Check if channel name is valid
	if (!isValidChannelName(channelName))
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("sent JOIN with invalid channel name: ") + RED + channelName + RESET);
		user->replyError(403, channelName, "No such channel");
		return false;
	}

	// Is User already in this channel?
	if (user->getChannels().count(channelName) > 0)
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to join already joined: ") + BLUE + channelName + RESET);
		user->replyError(443, channelName, "is already on channel");
		return false;
	}

	bool		wasCreated;
	Channel*	channel = server->getOrCreateChannel(channelName, user, key, &wasCreated);
	if (!channel)
		return false; // Creation failed, error already logged

	// Check if user can join the channel
	Channel::JoinResult	result;
	if (!channel->can_user_join(user->getNickname(), key, result))
	{
		switch (result)
		{
			case Channel::JOIN_INVITE_ONLY:
				logUserAction(user->getNickname(), user->getFd(), toString("tried to join invite-only channel ")
					+ BLUE + channelName + RESET + " without being invited");
				user->replyError(473, channelName, "Cannot join channel (+i)");
				break;

			case Channel::JOIN_FULL:
				logUserAction(user->getNickname(), user->getFd(),
					toString("tried to join full ") + BLUE + channelName + RESET);
				user->replyError(471, channelName, "Cannot join channel (+l)");
				break;

			case Channel::JOIN_BAD_KEY:
				logUserAction(user->getNickname(), user->getFd(),
					toString("tried to join channel ") + BLUE + channelName + RESET + " with bad key");
				user->replyError(475, channelName, "Cannot join channel (+k)");
				break;
		}
		return false;
	}

	// Add user to channel
	channel->add_user(user->getNickname());
	user->addChannel(channelName);

	// If user created the channel, make them an operator
	if (wasCreated)
	{
		channel->make_user_operator(user->getNickname());
		logUserAction(user->getNickname(), user->getFd(),
			toString("became operator of channel ") + BLUE + channelName + RESET);
	}	

	// Notify user(s) about successful join
	std::string	joinMessage =	":" + user->buildPrefix() + " JOIN :" + channelName;
	broadcastToChannel(server, channel, joinMessage);

	logUserAction(user->getNickname(), user->getFd(), toString("joined ") + BLUE + channelName + RESET);

	return true;
}

/**
Handles the IRC `JOIN` command, allowing a user to join one or more channels.

This function parses a comma-separated list of channel names and optional keys,
validates each, and attempts to join the user to each specified channel by calling
`handleSingleJoin()` for each pair.

Syntax:
	JOIN #chan1,#chan2 key1,key2
	JOIN #chan1,#chan2 :key1 with space, key2 with space

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the JOIN command.
 @param tokens	Parsed IRC command tokens (e.g., {"JOIN", "#chan1,#chan2", "key1,key2"}).

 @return		True if the command was processed (even if some joins failed),
				false if a critical error occurred.
*/
bool	Command::handleJoin(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (!checkRegistered(user, "JOIN"))
		return false;

	// Needs at least 2 tokens: JOIN <channel>
	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent JOIN without a channel name");
		user->replyError(461, "JOIN", "Not enough parameters");
		return false;
	}

	// Parse channels (comma-separated list)
	std::vector<std::string>	channels = splitCommaList(tokens[1]);
	std::vector<std::string>	keys;

	// Parse channel keys
	if (tokens.size() >= 3)
	{
		std::string	keyList = tokens[2];
		if (!keyList.empty() && keyList[0] == ':') // Check if key list starts with ':' (trailing parameter)
			keyList = keyList.substr(1); // Remove leading ':'
		keys = splitCommaList(keyList);
	}

	// Process each channel
	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::string&	channelName = channels[i];
		std::string		key = (i < keys.size()) ? keys[i] : "";

		handleSingleJoin(server, user, channelName, key);
	}
	return true;
}

/**
Handles a single `PART` command for one channel.

Validates channel name, membership, and removes the user from the channel.
Broadcasts the part message to all channel members.

 @param server		Pointer to the server instance.
 @param user		The user leaving the channel.
 @param channelName	Name of the channel to leave.
 @param partMessage	Optional part message.
 
 @return	True if the part succeeded, false otherwise.
*/
bool	Command::handleSinglePart(Server* server, User* user, const std::string& channelName, const std::string& partMessage)
{
	// Validate channel name format
	if (!isValidChannelName(channelName))
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("sent PART with invalid channel name: ") + RED + channelName + RESET);
		user->replyError(403, channelName, "No such channel");
		return false;
	}

	// Check if channel exists
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to leave non-existing ") + RED + channelName + RESET);
		user->replyError(403, channelName, "No such channel");
		return false;
	}

	// Check membership
	if (!channel->is_user_member(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(), toString("tried to leave ")
			+ BLUE + channelName + RESET + " but is not a member");
		user->replyError(442, channelName, "You're not on that channel");
		return false;
	}

	// Send PART message to all channel members (including the leaving user)
	std::string partLine = ":" + user->getNickname() + " PART " + channelName;
	if (!partMessage.empty())
		partLine += " :" + partMessage;

	broadcastToChannel(server, channel, partLine); // No exclusion - everyone gets the message

	// Remove user from the channel
	channel->remove_user(user->getNickname());
	user->removeChannel(channelName);

	logUserAction(user->getNickname(), user->getFd(), toString("left channel ") + BLUE + channelName
		+ RESET + (partMessage.empty() ? "" : toString(": ") + YELLOW + partMessage + RESET));

	return true;
}

/**
Handles the IRC `PART` command, allowing a user to leave one or more channels.

Syntax:
	PART #chan1,#chan2 [reason]

If multiple channels are given, the same reason is applied to all.

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the PART command.
 @param tokens	Parsed IRC command tokens (e.g., {"PART", "#chan1,#chan2", ":Bye"}).

 @return	True if the command was processed, false if critical error.
*/
bool Command::handlePart(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (!checkRegistered(user, "PART"))
		return false;

	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent PART without a channel name");
		user->replyError(461, "PART", "Not enough parameters");
		return false;
	}

	// Split channel list
	std::vector<std::string>	channels = splitCommaList(tokens[1]);

	// Extract reason (applied to all channels)
	std::string	partMessage = "";
	if (tokens.size() > 2)
	{
		for (size_t i = 2; i < tokens.size(); ++i)
		{
			if (i > 2)
				partMessage += " ";
			partMessage += tokens[i];
		}
		if (!partMessage.empty() && partMessage[0] == ':')
			partMessage = partMessage.substr(1);
	}

	// Process each channel
	for (size_t i = 0; i < channels.size(); ++i)
		handleSinglePart(server, user, channels[i], partMessage);

	return true;
}

/**
Handles the IRC `KICK` command, allowing a channel operator to remove a user
from a specified channel.

This function validates the channel and target user, checks that the
kicker has operator privileges, optionally extracts a kick reason, broadcasts
the `KICK` message to all channel members, and removes the target user from the channel.

Syntax:
	KICK #channel target [<reason>]

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the `KICK` command (must be a channel operator).
 @param tokens	Parsed IRC command tokens (e.g., {"KICK", "#channel", "victim", ":Spamming"}).

 @return		True if the command was processed and user was kicked,
				false if an error occurred.
*/
bool	Command::handleKick(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (tokens.size() < 3)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent KICK without enough parameters");
		user->replyError(461, "KICK", "Not enough parameters");
		return false;
	}

	const std::string&	channelName = tokens[1];
	const std::string&	targetNick = tokens[2];

	// Validate channel name format
	if (channelName.empty() || channelName[0] != '#')
	{
		logUserAction(user->getNickname(), user->getFd(), 
			toString("sent KICK with invalid channel name: ") + RED + channelName + RESET);
		user->replyError(403, channelName, "No such channel");
		return false;
	}

	// Check if channel exists
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		logUserAction(user->getNickname(), user->getFd(), 
			toString("tried to KICK from non-existing ") + RED + channelName + RESET);
		user->replyError(403, channelName, "No such channel");
		return false;
	}

	// Check if kicker is in the channel
	if (!channel->is_user_member(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(), 
			toString("tried to KICK from channel ") + BLUE + channelName + RESET + " but is not a member");
		user->replyError(442, channelName, "You're not on that channel");
		return false;
	}

	// Check if kicker has operator privileges
	if (!channel->is_user_operator(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(), 
			toString("tried to KICK from channel ") + BLUE + channelName + RESET + " but is not an operator");
		user->replyError(482, channelName, "You're not channel operator");
		return false;
	}

	// Check if target user exists
	User*	targetUser = server->getUser(targetNick);
	if (!targetUser)
	{
		logUserAction(user->getNickname(), user->getFd(), 
			toString("tried to KICK non-existing ") + RED + targetNick + RESET);
		user->replyError(401, targetNick, "No such nick/channel");
		return false;
	}

	// Check if target is in the channel
	if (!channel->is_user_member(targetNick))
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to KICK user ") + RED + targetNick + RESET + " who is not in channel " + BLUE + channelName + RESET);
		user->replyError(441, targetNick + " " + channelName, "They aren't on that channel");
		return false;
	}

	// Extract kick reason if provided
	std::string kickReason = "";
	if (tokens.size() > 3)
	{
		// Reconstruct the kick reason from tokens[3] onward
		for (size_t i = 3; i < tokens.size(); ++i)
		{
			if (i > 3)
				kickReason += " ";
			kickReason += tokens[i];
		}
		// Remove leading ':' if present (trailing parameter)
		if (!kickReason.empty() && kickReason[0] == ':')
			kickReason = kickReason.substr(1);
	}

	// Send KICK message to all channel members (including kicker and victim)
	std::string kickLine = ":" + user->getNickname() + " KICK " + channelName + " " + targetNick;
	if (!kickReason.empty())
		kickLine += " :" + kickReason;

	broadcastToChannel(server, channel, kickLine); // Everyone sees the kick

	// Remove target user from the channel
	channel->remove_user(targetNick);
	targetUser->removeChannel(channelName);

	logUserAction(user->getNickname(), user->getFd(),
		toString("kicked ") + GREEN + targetNick + RESET + " from channel " + BLUE + channelName
		+ RESET + (kickReason.empty() ? "" : toString(": ") + YELLOW + kickReason + RESET));

	return true;
}

/**
Handles the IRC `TOPIC` command, allowing a user to view or set the topic of a channel.

This function validates the channel existence and the user's membership, checks
for operator privileges if topic protection is enabled, and either sets a new
topic (broadcasting it to all channel members) or returns the current topic to
the requesting user.

Syntax:
	TOPIC #channel					; Request current topic
	TOPIC #channel :New topic text	; Set a new topic

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the `TOPIC` command.
 @param tokens	Parsed IRC command tokens (e.g., {"TOPIC", "#channel", ":New topic"}).

 @return		True if the command was processed successfully,
				false if an error occurred.
*/
bool	Command::handleTopic(Server *server, User *user, const std::vector<std::string> &tokens)
{
	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent TOPIC without a channel name\n");
		user->replyError(403, "", "No channel specified");
		return false;
	}

	const std::string&	channelName = tokens[1];
	Channel*			channel = server->getChannel(channelName);

	// Check if channel exists
	if (!channel)
	{
		logUserAction(user->getNickname(), user->getFd(), toString("tried to check/set topic for non-existing ")
			+ RED + channelName + RESET);
		user->replyError(403, channelName, "No such channel");
		return false;
	}

	// Check if user is a member of the channel
	if (!channel->is_user_member(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(), toString("tried to check/set topic for ")
			+ BLUE + channelName + RESET + " but is not a member");
		user->replyError(442, channelName, "You're not on that channel");
		return false;
	}

	// If a new topic is provided, attempt to set it
	if (tokens.size() > 2)
	{
		std::string	newTopic = tokens[2];
		if (newTopic[0] == ':')
			newTopic = newTopic.substr(1); // Remove leading ':'
		if (channel->has_topic_protection() && !channel->is_user_operator(user->getNickname()))
		{
			logUserAction(user->getNickname(), user->getFd(), toString("tried to set topic for ")
				+ BLUE + channelName + RESET + " but is not an operator");
			user->replyError(482, channelName, "You're not channel operator");
			return false;
		}
		channel->set_topic(newTopic);

		// Broadcast topic change to all channel members
		std::string	topicLine = ":" + user->getNickname() + " TOPIC " + channelName + " :" + newTopic;
		broadcastToChannel(server, channel, topicLine); // Everyone gets the topic change

		// Log the topic change
		logUserAction(user->getNickname(), user->getFd(), toString("set topic for ")
			+ BLUE + channelName + RESET + " to: " + YELLOW + newTopic + RESET);

		return true;
	}
	else
	{
		// No new topic provided, just show current topic
		std::string	currentTopic = channel->get_topic();
		if (currentTopic.empty())
			currentTopic = "(no topic)";

		// Send topic reply to user
		user->sendReply("332 " + user->getNickname() + " " + channelName + " :" + currentTopic);

		// Log the topic request
		logUserAction(user->getNickname(), user->getFd(),
			toString("queried topic for ") + BLUE + channelName + RESET);

		return true;
	}
}

/**
Handles the IRC `INVITE` command, allowing a user to invite another user to a channel.

This function validates the command syntax, checks user privileges, ensures
the target user exists and is not already in the channel, and sends the proper
replies to both the inviter and the invitee. It also handles adding the target
to the invite list for invite-only channels.

Syntax:
	INVITE <nickname> <channel>

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the `INVITE` command.
 @param tokens	Parsed IRC command tokens (e.g., {"INVITE", "targetNick", "#channel"}).

 @return		True if the command was successfully processed,
				false if an error occurred.
*/
bool	Command::handleInvite(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (tokens.size() < 3)
	{
		logUserAction(user->getNickname(), user->getFd(),
			"sent INVITE without enough arguments");
		user->replyError(461, "", "Not enough parameters");
		return false;
	}

	const std::string&	targetNick = tokens[1];
	const std::string&	channelName = tokens[2];

	// Check if channel exists
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to invite to non-existing ") + RED + channelName + RESET);
		user->replyError(403, channelName, "No such channel");
		return false;
	}

	// Check if user is on the channel (required to invite others)
	if (!channel->is_user_member(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to invite to channel ") + BLUE + channelName + RESET + " but is not a member");
		user->replyError(442, channelName, "You're not on that channel");
		return false;
	}

	// For invite-only channels, check if user is operator
	if (channel->is_invite_only() && !channel->is_user_operator(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to invite to invite-only ") + BLUE + channelName + RESET + " but is not an operator");
		user->replyError(482, channelName, "You're not channel operator");
		return false;
	}

	// Check if target user exists
	User*	targetUser = server->getUser(targetNick);
	if (!targetUser)
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to invite non-existing ") + RED + targetNick + RESET);
		user->replyError(401, targetNick, "No such nick/channel");
		return false;
	}

	// Check if target is already on the channel
	if (channel->is_user_member(targetNick))
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to invite already member ") + GREEN + targetNick + RESET + " to channel "
			+ BLUE + channelName + RESET);
		user->replyError(443, targetNick + " " + channelName, "is already on channel");
		return false;
	}

	// Add to invite list (for invite-only channels)
	if (channel->is_invite_only())
		channel->add_invite(targetNick);

	// send confirmation to inviter
	user->sendReply("341 " + user->getNickname() + " " + targetNick + " " + channelName);

	// send invitation to target user
	std::string	invitation = ":" + user->buildPrefix() + " INVITE " + targetNick + " :" + channelName;
	targetUser->getOutputBuffer() += invitation + "\r\n";

	// log the invite action
	logUserAction(user->getNickname(), user->getFd(),
		toString("invited ") + GREEN + targetNick + RESET + " to channel " + BLUE + channelName + RESET);

	return true;
}
