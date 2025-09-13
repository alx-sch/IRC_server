#include <sstream>		// std::ostringstream()

#include "../include/User.hpp"
#include "../include/Server.hpp"

/**
Appends the standard IRC welcome messages (numeric `001`–`004`) to user's output buffer
after a user successfully completes registration.

Source: https://dd.ircdocs.horse/refs/numerics/

 @param server	Pointer to the Server object (used to get server name, version, modes, etc.)
*/
void	User::replyWelcome()
{
	replyServerMsg("001 " + _nickname + " :Welcome to the " + _server->getNetwork()
		+ " Network, " + _nickname	 + "!" + _username + "@" + _host); // username@host might be not needed, check with HexChat

	replyServerMsg("002 " + _nickname + " :Your host is " + _server->getServerName()
		+ ", running version " + _server->getVersion());

	replyServerMsg("003 " + _nickname + " :This server was created " + _server->getCreationTime());

	replyServerMsg("004 " + _nickname + " " + _server->getServerName() + " "
		+ _server->getVersion() + " " + _server->getUModes() + " " + _server->getCModes());
}

/**
Appends an IRC numeric error to the user's output buffer (eventually flushed via `send()`).

 @param code		Numeric IRC error code (e.g. 464, 462, 433)
 @param param		Optional parameter for the error (e.g. nickname when invalid)
 @param message		Human-readable message to display
*/
void	User::replyError(int code, const std::string& param, const std::string& message)
{
	// If the user has no nickname yet, use '*'
	std::string			target = isRegistered() ? _nickname : "*";

	// Build message: <code> <target> [<param>] :<message>
	std::ostringstream	oss;
	oss << code << " " << target;
	if (!param.empty())
		oss << " " << param;
	oss << " :" << message;

	replyServerMsg(oss.str());
}

/**
Appends a raw IRC message from the server to the user's output buffer,
which is eventually flushed via `send()`.
Automatically prefixes the message with the server name and appends `\r\n`.

 @param message 	The already-formatted reply (e.g. "001 Alex :Welcome...")
*/
void	User::replyServerMsg(const std::string& message)
{
	if (_fd == -1) // User not connected
		return;

	std::string	fullMessage = ":" + _server->getServerName() + " " + message + "\r\n";
	_outputBuffer += fullMessage;
}
