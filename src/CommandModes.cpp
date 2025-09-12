#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidChannelName
#include "../include/defines.hpp"	// color formatting

/**
Handles the IRC `MODE` command.

This function processes both MODE queries and MODE changes:
 - Query:	If only the channel name is provided, it returns the current modes
			and relevant parameters (e.g., user limit, key for operators) to the user.
 - Change:	If mode flags and parameters are provided, changes are applied
 			via `handleModeChanges`.

The function validates that the user is registered, the target is a valid
channel, and that the user is a member of the channel. Sends appropriate
error replies if checks fail.

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the `MODE` command.
 @param tokens	Parsed IRC command tokens (e.g., {"MODE", "#channel", "+o", "user"}).

 @return		True if the command was processed successfully,
				false if an error occurred.
*/
bool	Command::handleMode(Server* server, User* user, const std::vector<std::string>& tokens)
{
	if (!checkRegistered(user, "MODE"))
		return false;

	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent MODE without parameters");
		user->replyError(461, "MODE", "Not enough parameters");
		return false;
	}

	// Check mode command on validity
	const std::string& target = tokens[1]; // target -> the channel the modes is to be applied to
	Channel*	channel = validateChannelAndUser(server, user, target);
	if (!channel)
		return false;

	// MODE query: just show current modes
	if (tokens.size() == 2)
	{
		std::string	modes;
		std::string	params;
		formatChannelModes(channel, user, modes, params);
		sendModeReply(user, server, target, modes, params);
		return true;
	}

	// MODE change: requires operator privileges
	if (!channel->is_user_operator(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(), toString("tried to change modes for ")
			+ BLUE + target + RESET + " but is not an operator");
		user->replyError(482, target, "You're not channel operator");
		return false;
	}

	return handleModeChanges(server, user, channel, tokens);
}

/**
Applies mode changes to a channel.

 @param server	Pointer to the server instance.
 @param user	The user issuing the MODE command.
 @param channel	The target channel.
 @param tokens	IRC command tokens.
 @return		True if the command processed successfully, false otherwise.
*/
bool Command::handleModeChanges(Server* server, User* user, Channel* channel, const std::vector<std::string>& tokens)
{
	const std::string&	modeString = tokens[2];
	size_t				paramIndex = 3;
	bool				adding = true;
	std::string			appliedModes;
	std::string			modeParams;

	for (size_t i = 0; i < modeString.length(); ++i)
	{
		char	mode = modeString[i];

		if (mode == '+') { adding = true; continue; }
		if (mode == '-') { adding = false; continue; }

		applyChannelMode(server, user, channel, mode, adding, tokens, paramIndex, appliedModes, modeParams);
	}

	// Broadcast mode changes if any were applied
	if (!appliedModes.empty())
		return true;

	std::string	modeMsg =	":" + user->getNickname() + " MODE " + channel->get_name()
							+ " " + appliedModes + modeParams;
	broadcastToChannel(server, channel, modeMsg);

	return true;
}

////////////
// HELPER //
////////////

/**
Validates the channel target and user membership for a `MODE` command.

Checks that the target is a valid channel name, the channel exists,
and the user is a member of the channel. Sends appropriate error replies
if any checks fail.

 @param server	Pointer to the server instance.
 @param user	The user issuing the MODE command.
 @param target	The channel name being targeted by the command.
 @return		Pointer to the Channel object if valid; nullptr otherwise.
*/
Channel*	Command::validateChannelAndUser(Server* server, User* user, const std::string& target)
{
	// Validate channel name format, only handle channel modes for now (no user modes)
	if (target.empty() || !isValidChannelName(target))
	{
		logUserAction(user->getNickname(), user->getFd(), toString("sent MODE for non-channel target: ")
			+ RED + target + RESET);
		user->replyError(501, "", "Unknown MODE flag");
		return NULL;
	}

	// Check if channel exists
	Channel*	channel = server->getChannel(target);
	if (!channel)
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("tried to change modes for non-existing ") + RED + target + RESET);
		user->replyError(403, target, "No such channel");
		return NULL;
	}

	// Check if user is in the channel
	if (!channel->is_user_member(user->getNickname()))
	{
		logUserAction(user->getNickname(), user->getFd(),
			toString("sent MODE but is not a member of ") + BLUE + target + RESET);
		user->replyError(442, target, "You're not on that channel");
		return NULL;
	}

	return channel;
}

/**
Builds the current mode string and parameter string for a channel.

Populates the 'modes' string with all set mode flags (e.g., "i", "t", "l", "k")
and the 'params' string with associated parameters (e.g., user limit, password).

 @param channel	The channel whose modes are being queried.
 @param user	The user requesting the mode information.
 @param modes	Reference to the string to populate with mode flags (prefixed with '+').
 @param params	Reference to the string to populate with any mode parameters.
*/
void	Command::formatChannelModes(Channel* channel, User* user, std::string& modes, std::string& params)
{
	modes.clear();
	params.clear();

	if (channel->is_invite_only())
		modes += "i";
	if (channel->has_topic_protection())
		modes += "t";
	if (channel->has_user_limit())
	{
		modes += "l";
		params += " " + toString(channel->get_user_limit());
	}
	if (channel->has_password())
	{
		modes += "k";
		if (channel->is_user_operator(user->getNickname())) // Only show key to channel operators
		params += " " + channel->get_password();
	}

	// Only prepend '+' if there are any modes set
	if (!modes.empty())
		modes = "+" + modes;
}

/**
Sends the `MODE` query reply to the user.

Constructs the IRC numeric reply (324) containing the channel's current modes
and any parameters, and appends it to the user's output buffer. Also logs the action.

 @param user	The user to send the reply to.
 @param server	Pointer to the server instance (for server name in reply).
 @param channel	The channel being queried.
 @param modes	Mode string (e.g., "+it") for the channel.
 @param params	Associated parameters (e.g., user limit, password).
*/
void	Command::sendModeReply(User* user, Server* server, const std::string& target, const std::string& modes,
			const std::string& params)
{
	user->getOutputBuffer()	+= ":" + server->getServerName() + " 324 " 
							+ user->getNickname() + " " + target
							+ (modes.empty() ? "" : " " + modes)
							+ params + "\r\n";

	logUserAction(user->getNickname(), user->getFd(), toString("queried modes for ") + BLUE + target + RESET);
}

// Applies a single mode change to a channel.
void	Command::applyChannelMode(Server* server, User* user, Channel* channel, char mode, bool adding,
									const std::vector<std::string>& tokens, size_t& paramIndex,
									std::string& appliedModes, std::string& modeParams)
{
	switch (mode)
	{
		case 'i':
		case 't':
			applySimpleMode(channel, user, mode, adding, appliedModes);
			break;

		case 'l':
			applyUserLimit(channel, user, adding, tokens, paramIndex, appliedModes, modeParams);
			break;

		case 'k':
			applyChannelKey(channel, user, adding, tokens, paramIndex, appliedModes, modeParams);
			break;

		case 'o':
			applyOperator(server, channel, user, adding, tokens, paramIndex, appliedModes, modeParams);
			break;

		default:
			logUserAction(user->getNickname(), user->getFd(), toString("tried to set unknown mode: ") + RED + mode + RESET);
			user->replyError(472, std::string(1, mode), "is unknown mode char to me");
			break;
	}
}

// Handles simple boolean modes like `i` and `t`.
void	Command::applySimpleMode(Channel* channel, User* user, char mode, bool adding, std::string& appliedModes)
{
	std::string	action;
	if (mode == 'i')
	{
		channel->set_invite_only(adding);
		action = "invite-only";
	}
	if (mode == 't')
	{
		channel->set_topic_protection(adding);
		action = "topic protection";
	}

	appliedModes += (adding ? "+" : "-");
	appliedModes += mode;

	logUserAction(user->getNickname(), user->getFd(),
		(adding ? "enabled " : "disabled ") + action + " for " + BLUE + channel->get_name() + RESET);
}

// Handles the `l` (user limit) mode.
void	Command::applyUserLimit(Channel* channel, User* user, bool adding, const std::vector<std::string>& tokens,
								size_t& paramIndex, std::string& appliedModes, std::string& modeParams)
{
	if (adding)
	{
		if (paramIndex < tokens.size())
		{
			int	limit = std::atoi(tokens[paramIndex].c_str());
			if (limit > 0)
			{
				channel->set_user_limit(limit);
				appliedModes += "+l";
				modeParams += " " + toString(limit);
				logUserAction(user->getNickname(), user->getFd(), toString("set user limit to ")
					+ YELLOW + toString(limit) + RESET + " for "
					+ BLUE + channel->get_name() + RESET);
			}
			++paramIndex;
		}
	}
	else
	{
		channel->set_user_limit(0);
		appliedModes += "-l";
		logUserAction(user->getNickname(), user->getFd(),
			toString("removed user limit for ") + BLUE + channel->get_name() + RESET);
	}
}

// Handles the `k` (channel key/password) mode.
void	Command::applyChannelKey(Channel* channel, User* user, bool adding, const std::vector<std::string>& tokens,
								size_t& paramIndex, std::string& appliedModes, std::string& modeParams)
{
	if (adding)
	{
		if (paramIndex < tokens.size())
		{
			const std::string&	key = tokens[paramIndex];
			channel->set_password(key);
			appliedModes += "+k";

			// Only operators see the actual key
			if (channel->is_user_operator(user->getNickname()))
				modeParams += " " + key;

			logUserAction(user->getNickname(), user->getFd(), toString("set channel key for ")
				+ BLUE + channel->get_name() + RESET);
			++paramIndex;
		}
	}
	else
	{
		channel->set_password("");
		appliedModes += "-k";
		logUserAction(user->getNickname(), user->getFd(), toString("removed channel key for ")
			+ BLUE + channel->get_name() + RESET);
	}
}

// Handles the `o` (operator) mode.
void	Command::applyOperator(Server* server, Channel* channel, User* user, bool adding,const std::vector<std::string>& tokens,
							size_t& paramIndex, std::string& appliedModes, std::string& modeParams)
{
	if (paramIndex >= tokens.size())
		return;

	const std::string&	targetNick = tokens[paramIndex];
	User*				targetUser = server->getUser(targetNick);

	if (!targetUser)
	{
		user->replyError(401, targetNick, "No such nick/channel");
		++paramIndex;
		return;
	}

	if (!channel->is_user_member(targetNick))
	{
		user->replyError(441, targetNick + " " + channel->get_name(), "They aren't on that channel");
		++paramIndex;
		return;
	}

	if (adding)
		channel->make_user_operator(targetNick);
	else
		channel->remove_user_operator_status(targetNick);

	appliedModes += (adding ? "+o" : "-o");	
	modeParams += " " + targetNick;
	logUserAction(user->getNickname(), user->getFd(), (adding ? "gave" : "removed") + toString(" operator status for ")
		+ GREEN + targetNick + RESET + " in " + BLUE + channel->get_name() + RESET);

	++paramIndex;
}
