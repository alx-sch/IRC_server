#include <sstream>		// std::ostringstream()
#include <sys/socket.h>	// send()

#include "../include/User.hpp"
#include "../include/Server.hpp"
#include "../include/defines.hpp"

/**
 Sends the standard IRC welcome messages (numeric `001`–`004`)
 after a user successfully completes registration.

 @param server 	Pointer to the Server object (used to get server name, version, modes, etc.)
*/
void	User::replyWelcome()
{
	sendReply("001 " + _nickname + " :Welcome to the " + NETWORK + " Network, "
		+ _nickname	 + "!" + _username + "@" + _host); // username@host might be not needed, check with HexChat

	sendReply("002 " + _nickname + " :Your host is " + _server->getServerName()
		+ ", running version " + VERSION);

	sendReply("003 " + _nickname + " :This server was created " + _server->getCreationTime());

	sendReply("004 " + _nickname + " " + _server->getServerName() + " " + VERSION
		+ " " + _server->getCModes() + " " + _server->getUModes());
}

/**
 Sends an IRC numeric error to the user.
 If the user is not yet registered, the target is replaced with `*`.

 @param code 		Numeric IRC error code (e.g. 464, 462, 433)
 @param message 	Human-readable message to display
*/
void	User::replyError(int code, const std::string& message)
{
	// If the user has no nickname yet, use '*'
	std::string			target = isRegistered() ? _nickname : "*";

	// Build message: <code> <target> :<message>
	std::ostringstream	oss;
	oss << code << " " << target << " :" << message;

	sendReply(oss.str());
}

/**
 Sends a raw IRC message to the user's socket.
 Automatically prefixes the message with the server name and appends `\r\n`.

 @param message 	The already-formatted reply (e.g. "001 Alex :Welcome...")
*/
void	User::sendReply(const std::string& message)
{
	if (_fd == -1) // User not connected
		return;

	std::string	fullMessage = ":" + _server->getServerName() + " " + message + "\r\n";
	if (send(_fd, fullMessage.c_str(), fullMessage.length(), 0) == -1)
		_server->handleSendError(_fd, _nickname);
}
