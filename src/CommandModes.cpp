#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidChannelName
#include "../include/defines.hpp"	// color formatting

bool	Command::handleMode(Server*, User*, const std::vector<std::string>&)
{
	/*
	The MODE command is used to set or change a channel's mode.
	*/
	return false; // TODO: Implement MODE functionality
}

