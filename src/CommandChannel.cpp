#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// isValidChannelName
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
		user->logUserAction(toString("sent JOIN with invalid channel name: ") + RED + channelName + RESET);
		user->sendError(403, channelName, "No such channel");
		return false;
	}

	// Is User already in this channel?
	if (user->getChannels().count(normalize(channelName)) > 0)
	{
		Channel*	existingChannel = server->getChannel(channelName);

		user->logUserAction(toString("tried to join already joined ")
			+ BLUE + existingChannel->get_name() + RESET);
		user->sendError(443, existingChannel->get_name(), "is already on channel");
		return false;
	}

	bool		wasCreated;
	Channel*	channel = server->getOrCreateChannel(channelName, user, &wasCreated);
	if (!channel)
		return false; // Creation failed, error already logged
	std::string	channelNameOrig = channel->get_name();

	// Check if user can join the channel
	Channel::JoinResult	result;
	if (!channel->can_user_join(user, key, result))
	{
		switch (result)
		{
			case Channel::JOIN_INVITE_ONLY:
				user->logUserAction(toString("tried to join invite-only channel ")
					+ BLUE + channelNameOrig + RESET + " without being invited");
				user->sendError(473, channelNameOrig, "Cannot join channel (+i)");
				break;

			case Channel::JOIN_FULL:
				user->logUserAction(toString("tried to join full ") + BLUE + channelNameOrig + RESET);
				user->sendError(471, channelNameOrig, "Cannot join channel (+l)");
				break;

			case Channel::JOIN_BAD_KEY:
				user->logUserAction(toString("tried to join channel ") + BLUE + channelNameOrig + RESET
					+ " with bad key");
				user->sendError(475, channelNameOrig, "Cannot join channel (+k)");
				break;
			case Channel::JOIN_MAX_CHANNELS:
				user->logUserAction(toString("tried to join ") + BLUE + channelNameOrig + RESET
					+ " but is already in too many channels");
				user->sendError(405, channelNameOrig, "You have joined too many channels");
				break;
		}
		return false;
	}

	// Add user to channel
	channel->add_user(user);
	user->addChannel(channelName);

	// If user created the channel, make them an operator
	if (wasCreated)
	{
		channel->make_user_operator(user);
		user->logUserAction(toString("became operator of ") + BLUE + channelName + RESET);

		// Make channel password protected if key was provided
		if (!key.empty())
		{
			channel->set_password(key);
			user->logUserAction(toString("set channel key for ") + BLUE + channelName + RESET);
		}

		// Have the bot join the channel as well and make it an operator
		if (server->getBotMode())
		{
			channel->add_user(server->getBotUser());
			server->getBotUser()->addChannel(channelName);
			channel->make_user_operator(server->getBotUser());
		}
	}	

	// Notify user(s) about successful join
	std::string	joinMessage =	":" + user->buildHostmask() + " JOIN :" + channelNameOrig;
	broadcastToChannel(channel, joinMessage);

	// Send channel topic to the joining user
	if (channel->get_topic().empty())
		user->sendServerMsg(toString("331 ") + user->getNickname() + " " + channelNameOrig + " :No topic is set");

	else
		user->sendServerMsg(toString("332 ") + user->getNickname() + " " + channelNameOrig + " :" + channel->get_topic());

	// Send channel mode to the joining user
	std::string	modeString = channel->get_mode_string(user);
	user->sendServerMsg(toString("324 ") + user->getNickname() + " " + channelNameOrig + " " + modeString);

	// Send names list to the joining user
	std::string	namesList = channel->get_names_list();
	user->sendServerMsg(toString("353 ") + user->getNickname() + " = " + channelNameOrig + " :" + namesList);
	user->sendServerMsg(toString("366 ") + user->getNickname() + " " + channelNameOrig + " :End of /NAMES list");

	user->logUserAction(toString("joined ") + BLUE + channelNameOrig + RESET);

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
		user->logUserAction("sent JOIN without a channel name");
		user->sendError(461, "JOIN", "Not enough parameters");
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

		bool usrJoined = handleSingleJoin(server, user, channelName, key);

		if (server->getBotMode() && usrJoined)
		{
			handleMessageToUser(server, server->getBotUser(), user->getNicknameLower(),
				"Welcome to " + channelName + ", dear " + user->getNickname() + "!", "NOTICE");
			handleMessageToUser(server, server->getBotUser(), user->getNicknameLower(),
				"I am a friendly IRCbot and I'm pleased to meet you!", "NOTICE");
			handleMessageToUser(server, server->getBotUser(), user->getNicknameLower(),
				"Use command 'joke' or 'calc <expression>' (e.g. 'calc 40 + 2', int only) and see what happens!", "NOTICE");
		}
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
	if (!checkRegistered(user, "PART"))
		return false;

	// Validate channel name format
	if (!isValidChannelName(channelName))
	{
		user->logUserAction(toString("sent PART with invalid channel name: ") + RED + channelName + RESET);
		user->sendError(403, channelName, "No such channel");
		return false;
	}

	// Check if channel exists
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		user->logUserAction(toString("tried to leave non-existing ") + RED + channelName + RESET);
		user->sendError(403, channelName, "No such channel");
		return false;
	}

	std::string	channelNameOrig = channel->get_name();

	// Check membership
	if (!channel->is_user_member(user))
	{
		user->logUserAction(toString("tried to leave ") + BLUE + channelNameOrig + RESET
			+ " but is not a member");
		user->sendError(442, channelNameOrig, "You're not on that channel");
		return false;
	}

	// Send PART message to all channel members (including the leaving user)
	std::string	partLine = ":" + user->buildHostmask() + " PART " + channelNameOrig;
	if (!partMessage.empty())
		partLine += " :" + partMessage;

	broadcastToChannel(channel, partLine); // No exclusion - everyone gets the message

	// Remove user from the channel
	channel->remove_user(user);
	user->removeChannel(channelName);
	user->logUserAction(toString("left channel ") + BLUE + channelNameOrig + RESET
		+ (partMessage.empty() ? "" : toString(": ") + YELLOW + partMessage + RESET));

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
		user->logUserAction("sent PART without a channel name");
		user->sendError(461, "PART", "Not enough parameters");
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
	{
		handleSinglePart(server, user, channels[i], partMessage);

		// BOT MODE: If channel has no active users except bot - it removes the channel.
		if (server->getBotMode() && server->getChannel(channels[i])->get_connected_user_number() == 1)
		{
			server->getBotUser()->removeChannel(channels[i]);
			server->deleteChannel(channels[i], "no connected users");
		}

		// If channel has no active users - it removes the channel. 
		else if (!server->getChannel(channels[i])->get_connected_user_number())
			server->deleteChannel(channels[i], "no connected users");
	}

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
	if (!checkRegistered(user, "KICK"))
		return false;

	if (tokens.size() < 3)
	{
		user->logUserAction("sent KICK without enough parameters");
		user->sendError(461, "KICK", "Not enough parameters");
		return false;
	}

	const std::string&	channelName = tokens[1];
	std::string			targetNickOrig = tokens[2];

	// Validate channel name format
	if (channelName.empty() || channelName[0] != '#')
	{
		user->logUserAction(toString("sent KICK with invalid channel name: ") + RED + channelName + RESET);
		user->sendError(403, channelName, "No such channel");
		return false;
	}

	// Check if channel exists
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		user->logUserAction(toString("tried to KICK from non-existing ") + RED + channelName + RESET);
		user->sendError(403, channelName, "No such channel");
		return false;
	}

	std::string	channelNameOrig = channel->get_name();

	// Check if kicker is in the channel
	if (!channel->is_user_member(user))
	{
		user->logUserAction(toString("tried to KICK from channel ") + BLUE + channelNameOrig + RESET
			+ " but is not a member");
		user->sendError(442, channelNameOrig, "You're not on that channel");
		return false;
	}

	// Check if kicker has operator privileges
	if (!channel->is_user_operator(user))
	{
		user->logUserAction(toString("tried to KICK from channel ") + BLUE + channelNameOrig + RESET
			+ " but is not an operator");
		user->sendError(482, channelNameOrig, "You're not channel operator");
		return false;
	}

	// Check if target user exists
	User*	targetUser = server->getUser(normalize(targetNickOrig));
	if (!targetUser)
	{
		user->logUserAction(toString("tried to KICK non-existing ") + RED + targetNickOrig + RESET);
		user->sendError(401, targetNickOrig, "No such nick/channel");
		return false;
	}

	// Check if target is in the channel
	if (!channel->is_user_member(targetUser))
	{
		user->logUserAction(toString("tried to KICK user ") + GREEN + targetUser->getNickname() + RESET
			+ " who is not in " + BLUE + channelNameOrig + RESET);
		user->sendError(441, targetUser->getNickname() + " " + channelNameOrig, "They aren't on that channel");
		return false;
	}

	// Check if user to be kicked is also an operator (bot is always operator)
	if (channel->is_user_operator(targetUser))
	{
		user->logUserAction(toString("tried to KICK operator ") + GREEN + targetUser->getNickname() + RESET
			+ " from " + BLUE + channelNameOrig + RESET);
		user->sendError(482, channelNameOrig, "Cannot kick another channel operator");
		return false;
	}

	// Extract kick reason if provided
	std::string	kickReason = "";
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
	std::string	kickLine = ":" + user->buildHostmask() + " KICK " + channelNameOrig + " " + targetUser->getNickname();
	if (!kickReason.empty())
		kickLine += " :" + kickReason;

	broadcastToChannel(channel, kickLine); // Everyone sees the kick

	// Remove target user from the channel
	channel->remove_user(targetUser);
	targetUser->removeChannel(channelName);

	user->logUserAction(toString("kicked ") + GREEN + targetUser->getNickname() + RESET + " from channel "
		+ BLUE + channelNameOrig + RESET + (kickReason.empty() ? "" : toString(": ") + YELLOW + kickReason + RESET));

	// BOT MODE: If channel has no active users except bot - it removes the channel.
	if (server->getBotMode() && channel->get_connected_user_number() == 1)
	{
		server->getBotUser()->removeChannel(channelNameOrig);
		server->deleteChannel(channelNameOrig, "no connected users");
	}

	// If the kicked user was the last one, delete the channel
	else if (!channel->get_connected_user_number())
		server->deleteChannel(channelNameOrig, "no connected users");

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
	if (!checkRegistered(user, "TOPIC"))
		return false;

	if (tokens.size() < 2)
	{
		user->logUserAction("sent TOPIC without a channel name");
		user->sendError(403, "", "No channel specified");
		return false;
	}

	const std::string&	channelName = tokens[1];
	Channel*			channel = server->getChannel(channelName);

	// Check if channel exists
	if (!channel)
	{
		user->logUserAction(toString("tried to check/set topic for non-existing ") + RED + channelName + RESET);
		user->sendError(403, channelName, "No such channel");
		return false;
	}

	std::string	channelNameOrig = channel->get_name();

	// Check if user is a member of the channel
	if (!channel->is_user_member(user))
	{
		user->logUserAction(toString("tried to check/set topic for ") + BLUE + channelNameOrig + RESET
			+ " but is not a member");
		user->sendError(442, channelNameOrig, "You're not on that channel");
		return false;
	}

	// If a new topic is provided, attempt to set it
	if (tokens.size() > 2)
	{
		std::string	newTopic = tokens[2];
		if (newTopic[0] == ':')
			newTopic = newTopic.substr(1); // Remove leading ':', if present
		if (channel->has_topic_protection() && !channel->is_user_operator(user))
		{
			user->logUserAction(toString("tried to set topic for ") + BLUE + channelNameOrig + RESET
				+ " but is not an operator");
			user->sendError(482, channelNameOrig, "You're not channel operator");
			return false;
		}
		channel->set_topic(newTopic, user->buildHostmask());

		// Broadcast topic change to all channel members
		std::string	topicLine = ":" + user->buildHostmask() + " TOPIC " + channelNameOrig + " :" + newTopic;
		broadcastToChannel(channel, topicLine); // Everyone gets the topic change

		// Log the topic change
		user->logUserAction(toString("set topic for ") + BLUE + channelNameOrig + RESET
			+ " to: " + YELLOW + newTopic + RESET);

		return true;
	}
	else // No new topic provided, just show current topic
	{
		std::string	currentTopic = channel->get_topic();
		if (currentTopic.empty())
		{
			user->sendServerMsg("331 " + user->getNickname() + " " + channelNameOrig + " :No topic is set");
		}
		else
		{
			// Send topic reply to user
			user->sendServerMsg("332 " + user->getNickname() + " " + channelNameOrig + " :" + currentTopic);
			user->sendServerMsg("333 " + user->getNickname() + " " + channelNameOrig + " "
				+ channel->get_topic_set_info());
		}

		// Log the topic request
		user->logUserAction(toString("queried topic for ") + BLUE + channelNameOrig + RESET);

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
	if (!checkRegistered(user, "INVITE"))
		return false;

	if (tokens.size() < 3)
	{
		user->logUserAction("sent INVITE without enough arguments");
		user->sendError(461, "", "Not enough parameters");
		return false;
	}

	std::string	targetNickOrig = tokens[1];
	const std::string&	channelName = tokens[2];

	// Check if channel exists
	Channel*	channel = server->getChannel(channelName);
	if (!channel)
	{
		user->logUserAction(toString("tried to invite to non-existing ") + RED + channelName + RESET);
		user->sendError(403, channelName, "No such channel");
		return false;
	}

	std::string	channelNameOrig = channel->get_name();

	// Check if user is on the channel (required to invite others)
	if (!channel->is_user_member(user))
	{
		user->logUserAction(toString("tried to invite to ") + BLUE + channelNameOrig + RESET
			+ " but is not a member");
		user->sendError(442, channelNameOrig, "You're not on that channel");
		return false;
	}

	// For invite-only channels, check if user is operator
	if (channel->is_invite_only() && !channel->is_user_operator(user))
	{
		user->logUserAction(toString("tried to invite to invite-only ") + BLUE + channelNameOrig + RESET
			+ " but is not an operator");
		user->sendError(482, channelNameOrig, "You're not channel operator");
		return false;
	}

	// Check if target user exists
	User*	targetUser = server->getUser(normalize(targetNickOrig));
	if (!targetUser)
	{
		user->logUserAction(toString("tried to invite non-existing ") + RED + targetNickOrig + RESET);
		user->sendError(401, targetNickOrig, "No such nick/channel");
		return false;
	}

	// Check if target is already on the channel
	if (channel->is_user_member(targetUser))
	{
		user->logUserAction(toString("tried to invite already member ") + GREEN + targetUser->getNickname() + RESET
			+ " to " + BLUE + channelNameOrig + RESET);
		user->sendError(443, targetUser->getNickname() + " " + channelNameOrig, "is already on channel");
		return false;
	}

	// Add to invite list (for invite-only channels)
	if (channel->is_invite_only())
		channel->add_invite(targetNickOrig); // Normalized inside add_invite

	// send confirmation to inviter
	user->sendServerMsg("341 " + user->getNickname() + " " + targetUser->getNickname() + " " + channelNameOrig);

	// send invitation to target user
	targetUser->sendMsgFromUser(user, "INVITE " + targetUser->getNickname() + " :" + channelNameOrig);

	// log the invite action
	user->logUserAction(toString("invited ") + GREEN + targetUser->getNickname() + RESET
		+ " to " + BLUE + channelNameOrig + RESET);

	return true;
}

/**
Handles the IRC `LIST` command, displaying a list of the server's channels.

As opposed to the regular `LIST` command which behaves differently depending
on if it's parameterized or not - this function will behave the same way
regardless of if arguments are provided or not.
It displays the different channels, the amount of connected users in each channel,
and the topic of the channel (if any).

Syntax:
	LIST

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the `LIST` command.
 @param tokens	Parsed IRC command tokens "LIST".

 @return		True if the command was successfully processed,
				false if an error occurred.
*/
bool	Command::handleList(Server* server, User* user)
{
	if (!checkRegistered(user, "LIST"))
		return false;

	user->logUserAction("sent valid LIST command");
	user->sendServerMsg("321 " + user->getNickname() + " Channel :Users Name"); // Start of list

	std::map<std::string, Channel*> channels = server->getAllChannels();

	std::map<std::string, Channel*>::const_iterator	it;
	std::map<std::string, Channel*>::const_iterator	ite = channels.end();

	for (it = channels.begin(); it != ite; it++) // Iterates through each channel and sends to user.
	{
		user->sendServerMsg("322 " + user->getNickname() + " " + it->second->get_name() + " " 
			+ toString(it->second->get_connected_user_number()) + " :" + it->second->get_topic());
	}

	user->sendServerMsg("323 " + toString(user->getNickname()) + " :End of /LIST"); // End of list.

	return true;
}
