#include <vector>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"		// logUserAction, isValidChannelName
#include "../include/defines.hpp"	// color formatting

/**
Handles the IRC `MODE` command.

XXXXX

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the `MODE` command.
 @param tokens	Parsed IRC command tokens (e.g., {"MODE", "#channel", "+o", "user"}).

 @return		True if the command was processed successfully,
				false if an error occurred.
*/
bool	Command::handleMode(Server*, User*, const std::vector<std::string>&)
{
	/*
	The MODE command is used to set or change a channel's mode.
	*/
	return false; // TODO: Implement MODE functionality
}

