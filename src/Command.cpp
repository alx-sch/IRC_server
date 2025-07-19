#include <sstream>
#include "../include/Command.hpp"

/**
 Tokenizes a raw IRC message into space-separated parts,
 preserving the trailing parameter(after a colon `:`).
 
 This function splits an IRC line like:
	"USER max 0 * :Max Power the Third"
 into:
 	["USER", "max", "0", "*", "Max Power the Third"]

 If a token starts with a colon (`:`), the rest of the line (including spaces) is treated
 as a single argument (the trailing parameter), as per IRC protocol.

 @param message 	The raw IRC message line.
 @return 			A vector of tokens: command + arguments (with trailing combined).
*/
std::vector<std::string>	Command::tokenize(const std::string& message)
{
	std::string					token;
	std::vector<std::string>	tokens;
	std::istringstream			iss(message);	// Convert string to stream for easy tokenization

	// Extract tokens by whitespace ('iss >>' skips repeated spaces)
	while (iss >> token)
	{
		// If token starts with ':', treat rest of line as one token
		// Assume "USER max 0 * :Max Power the Third"
		if (token[0] == ':') // token: ":Max"
		{
			std::string	trailing = token.substr(1); // strip leading ':' -> token: "Max"
			std::string	rest;
			std::getline(iss, rest); // read rest of the line -> rest: " Power the Third"
			tokens.push_back(trailing + rest); // Combine and add to tokens -> tokens: ["Max Power the Third"]
			break; // After processing ':', stop reading more tokens (':' indicates last token)
		}
		else
			tokens.push_back(token);
	}
	return tokens;
}

// Extracts the command type from a message
// Returns `UNKNOWN` if no valid command is found
Command::Type Command::getType(const std::string& message)
{
	std::vector<std::string>	tokens = tokenize(message);
	if (tokens.empty())
		return UNKNOWN;

	const std::string&			cmd = tokens[0];

	if (cmd == "NICK")
		return NICK;
	if (cmd == "USER")
		return USER;
	if (cmd == "PASS")
		return PASS;
	if (cmd == "PING")
		return PING;
	if (cmd == "JOIN")
		return JOIN;
	if (cmd == "PART")
		return PART;
	if (cmd == "QUIT")
		return QUIT;
	if (cmd == "PRIVMSG")
		return PRIVMSG;
	if (cmd == "TOPIC")
		return TOPIC;
	if (cmd == "KICK")
		return KICK;
	if (cmd == "INVITE")
		return INVITE;
	if (cmd == "MODE")
		return MODE;

	return UNKNOWN;
}


