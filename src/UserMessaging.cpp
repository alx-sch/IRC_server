#include <sstream>		// std::ostringstream()

#include "../include/User.hpp"
#include "../include/Server.hpp"

/**
Appends the standard IRC welcome messages (numeric `001`–`004`) to user's output buffer
after a user successfully completes registration.

Source: https://dd.ircdocs.horse/refs/numerics/

 @param server	Pointer to the Server object (used to get server name, version, modes, etc.)
*/
void	User::sendWelcome()
{
	sendServerMsg("001 " + _nickname + " :Welcome to the " + _server->getNetwork()
		+ " Network, " + _nickname	 + "!" + _username + "@" + _host); // username@host might be not needed, check with HexChat

	sendServerMsg("002 " + _nickname + " :Your host is " + _server->getServerName()
		+ ", running version " + _server->getVersion());

	sendServerMsg("003 " + _nickname + " :This server was created " + _server->getCreationTime());

	sendServerMsg("004 " + _nickname + " " + _server->getServerName() + " "
		+ _server->getVersion() + " " + _server->getUModes() + " " + _server->getCModes());
}

/**
Appends an IRC numeric error to the user's output buffer (eventually flushed via `send()`).

 @param code		Numeric IRC error code (e.g. 464, 462, 433)
 @param param		Optional parameter for the error (e.g. nickname when invalid)
 @param message		Human-readable message to display
*/
void	User::sendError(int code, const std::string& param, const std::string& message)
{
	std::ostringstream	oss;
	std::string			target = isRegistered() ? _nickname : "*";	// If no nickname yet, use '*'

	// Build message: <code> <target> [<param>] :<message>
	oss << code << " " << target;
	if (!param.empty())
		oss << " " << param;
	oss << " :" << message;

	sendServerMsg(oss.str());
}

/**
Appends a raw IRC message from the server to the user's output buffer,
which is eventually flushed via `send()`.
Automatically prefixes the message with the server name and appends `\r\n`.

 @param message	The already-formatted reply (e.g. "001 Alex :Welcome...")
*/
void	User::sendServerMsg(const std::string& message)
{
	if (_fd == -1) // User not connected
		return;

	std::string	fullMessage = ":" + _server->getServerName() + " " + message + "\r\n";
	_outputBuffer += fullMessage;
}

/**
Appends a raw IRC message from another user to this user's output buffer.
Automatically prefixes the message with the sender's hostmask and appends `\r\n`.

 @param sender	The User who is the origin of the message.
 @param message	The already-formatted command and parameters (e.g., "KICK #chan Bob :reason")
*/
void	User::sendMsgFromUser(const User* sender, const std::string& message)
{
	if (_fd == -1 || sender == NULL) // This user or sender not connected
		return;

	std::string	fullMessage = ":" + sender->buildHostmask() + " " + message + "\r\n";
	_outputBuffer += fullMessage;
}
