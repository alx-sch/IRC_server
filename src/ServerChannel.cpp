#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logServerMessage
#include "../include/defines.hpp"	// color formatting

/**
Retrieves an `Channel` object by its name.

 @param channelName	The name of the channel to retrieve.
 @return			Pointer to the `Channel` object if found, `NULL` otherwise.
*/
Channel*	Server::getChannel(const std::string& channelName) const
{
	std::map<std::string, Channel*>::const_iterator	it = _channels.find(channelName);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

/**
Retrieves an existing `Channel` by name or creates a new one if it does not exist.

If the channel with the given name exists, a pointer to it is returned.
Otherwise, a new `Channel` object is created, added to the server, and returned.
In case of memory allocation failure, an error is logged and the user is notified.

 @param channelName	The name of the channel to retrieve or create.
 @param user		The user requesting or triggering the channel creation.
 @param key			Optional channel key to set if creating a new channel.
 @param wasCreated	Optional output parameter set to true if a new channel was created, false otherwise.

 @return			Pointer to the `Channel` object, or `NULL` on failure.
*/
Channel*	Server::getOrCreateChannel(const std::string& channelName, User* user, const std::string& key, bool* wasCreated)
{
	Channel*	channel = getChannel(channelName);
	if (channel)
	{
		if (wasCreated)
			*wasCreated = false;
		return channel;
	}

	// Create a new channel if it does not exist
	try
	{
		channel = new Channel(channelName);
		if (!key.empty()) // If a key is provided, set it as the channel password
			channel->set_password(key);
		_channels[channelName] = channel; // Add to the server's channel map
		logUserAction(user->getNickname(), user->getFd(), std::string("created ")
			+ BLUE + channelName + RESET);

		if (wasCreated)
			*wasCreated = true;
	}
	catch(const std::bad_alloc&)
	{
		logUserAction(user->getNickname(), user->getFd(), RED
			+ std::string("ERROR: Failed to allocate memory for new channel ") + BLUE + channelName + RESET);

		user->replyError(500, "", "Internal server error while creating channel " + channelName);
		if (wasCreated)
			*wasCreated = false;
		return NULL;
	}
	return channel;
}

// Deletes a channel by name, frees its memory, removes it from the map, and logs the reason.
void	Server::deleteChannel(const std::string& channelName, std::string reason)
{
	std::map<std::string, Channel*>::iterator	it = _channels.find(channelName);
	if (it != _channels.end())
	{
		logServerMessage(std::string("Channel ") + BLUE + channelName + RESET
			+ " deleted (" + reason + ")");
		delete it->second;		// Free memory for the channel
		_channels.erase(it);	// Remove from the map
	}
}
