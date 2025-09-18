#include "../include/utils.hpp"		// toString()
#include "../include/defines.hpp"	// MAX_NICK_LENGTH, color formatting

#include <iostream>		// std::cout
#include <ctime>		// time_t, gmtime, strftime
#include <stdexcept>	// std::runtime_error
#include <iomanip>		// std::setw, std::left, std::right
#include <cctype>		// For ::isalpha(), ::isdigit()
#include <cstdlib>		// strtol
#include <algorithm>	// std::transform

// Parses and validates a port number gitfrom a C-style string (argument)
int	parsePort(const char* arg)
{
	char*	end;
	long	port = std::strtol(arg, &end, 10);

	// Check:
	// - if the entire string was parsed (*end != '\0' means garbage after number)
	// - valid port range -> max. 16-bit unsigned int: 65535
	if (*end != '\0' || port <= 0 || port > 65535)
		throw std::runtime_error("Invalid port number: " + toString(arg) + " (must be between 1 and 65535)");

	return static_cast<int>(port);
}

/**
Returns the current time formatted as a readable string.
Used in server welcome message.

Example output:
	`Fri Jul 19 2025 at 21:47:30 UTC`

 @return	A string containing the current date and time in UTC format.
*/
std::string	getFormattedTime()
{
	std::time_t	now = std::time(NULL);		// Get current time as time_t (seconds since epoch)
	std::tm*	gmt = std::gmtime(&now);	// Convert to UTC time (struct tm)
	char		buffer[128];

	// Format: AbbrWeekday AbbrMonth Day Year at HH:MM:SS UTC
	std::strftime(buffer, sizeof(buffer), "%a %b %d %Y at %H:%M:%S UTC", gmt);

	return std::string(buffer);
}

/**
Returns the current UTC formatted as a readable string.
Used in server logging.

Example output:
	`2025-08-03 18:47:39`

 @return	A string containing the current date and time in UTC format.
*/
std::string	getTimestamp()
{
	std::time_t	now = std::time(NULL);		// Get current time as time_t (seconds since epoch)
	std::tm*	gmt = std::gmtime(&now);	// Convert to UTC time (struct tm)
	char		buffer[128];

	// Format: YYYY-MM-DD HH:MM:SS UTC
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmt);

	return std::string(buffer);
}

// Checks if a character is a letter (`a-z`, `A-Z`)
// Returns true if the character is a letter, false otherwise.
static bool	isLetter(char c)
{
	return std::isalpha(static_cast<unsigned char>(c));
}

// Checks if a character is a digit (`0-9`)
// Returns true if the character is a digit, false otherwise.
static bool	isDigit(char c)
{
	return std::isdigit(static_cast<unsigned char>(c));
}

// Checks if a character is a special character
// (`-`, `[`, `]`, `\`, `` ` ``, `^`, `{`, `}`).
// Returns true if the character is a special character, false otherwise.
static bool	isSpecial(char c)
{
	switch (c)
	{
		case '-':
		case '[':
		case ']':
		case '\\':
		case '`':
		case '^':
		case '{':
		case '}':
			return true;
		default:
			return false;
	}
}

/**
Check if the nickname is valid according to IRC rules
 - Must not be empty
 - Max length is 9 characters
 - Must start with a letter
 - Can contain letters, digits, and special characters

See RFC 1459, section 2.3.1:
	<nick> ::= <letter> { <letter> | <number> | <special> }
	<special> ::= '-' | '[' | ']' | '\' | '`' | '^' | '{' | '}'
*/
bool	isValidNick(const std::string& nick)
{
	if (nick.empty() || nick.length() > MAX_NICK_LENGTH)
		return false;

	if (!isLetter(nick[0])) // IRC rule: must start with a letter
		return false;

	for (unsigned int i = 1; i < nick.length(); ++i)
	{
		char	c = nick[i];
		if (!isLetter(c) && !isDigit(c) && !isSpecial(c))
			return false;
	}
	return true;
}

/**
Check if the channel name is valid according to IRC rules
 - Must not be empty
 - Max length is 200 characters (can be shorter, defined by server)
 - Must start with '#' or '&'
 - String after prefix cannot be empty
 - May contain any characters except:
	space, comma, ASCII bell (^G), null, carriage return, or line feed

See RFC 1459, section 2.3.1:
	<channel> ::= ('#' | '&') <chstring>
	<chstring> ::= any 8bit except SPACE, BELL, NUL, CR, LF, comma
*/
bool	isValidChannelName(const std::string& channelName)
{
	if (channelName.length() < 2 || channelName.length() > MAX_CHANNEL_LENGTH)
		return false;

	if (channelName[0] != '#' && channelName[0] != '&')
		return false;

	// Check each character in the channel name (after prefix)
	for (size_t i = 1; i < channelName.length(); ++i)
	{
		char	c = channelName[i];

		// Reject SPACE, BELL, NUL, CR, LF, and comma
		 if (c == ' ' || c == '\a' || c == '\0' || c == '\r' || c == '\n' || c == ',')
			return false;
	}
	return true;
}

/**
Formats a log line with timestamp, aligned nickname and fd columns.

 @param nick	The user's nickname
 @param fd		The user's socket fd
 @param message	The message to log
*/
void	logUserAction(const std::string& nick, int fd, const std::string& message)
{
	std::ostringstream	oss;

	oss	<< "[" << CYAN << getTimestamp() << RESET << "] "
		<< GREEN << std::left << std::setw(MAX_NICK_LENGTH + 1) << nick << RESET // pad nick + some space
		<< "(" << MAGENTA << "fd " << std::right << std::setw(3) << fd << RESET << ") "
		<< message;

	std::cout << oss.str() << std::endl;
}

// Logs a general server message with timestamp.
void	logServerMessage(const std::string& message)
{
	std::cout	<< "[" << CYAN << getTimestamp() << RESET << "] "
				<< std::left << std::setw(MAX_NICK_LENGTH + 10) << " " // pad for alignment with user logs
				<< message << std::endl;
}

// IRC-specific case mapping for lowercase conversion.
// See RFC 1459, section 2.2.2
static char	ircToLowerChar(char c)
{
	if (c >= 'A' && c <= 'Z') return c + 32; // A-Z → a-z
	if (c == '[') return '{';
	if (c == ']') return '}';
	if (c == '\\') return '|';
	if (c == '~') return '^';
	return c;
}


// Normalizes a nicknames or channels for case-insensitive storage and lookup
// lowercase letters and certain special characters are mapped
// to their lowercase equivalents as per IRC case-mapping rules.
std::string	normalize(const std::string& name)
{
	std::string	result = name;
	std::transform(result.begin(), result.end(), result.begin(), ircToLowerChar);
	return result;
}
