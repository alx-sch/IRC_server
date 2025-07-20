#include "../include/utils.hpp"

// Parses and validates a port number from a C-style string (argument)
int	parsePort(const char* arg)
{
	char*	end;
	long	port = std::strtol(arg, &end, 10);

	// Check:
	// - if the entire string was parsed (*end != '\0' means garbage after number)
	// - valid port range -> max. 16-bit unsigned int: 65535
	if (*end != '\0' || port <= 0 || port > 65535)
		throw std::runtime_error("Invalid port number: " + std::string(arg) + " (must be between 1 and 65535)");

	return static_cast<int>(port);
}

/**
 Returns the current server time formatted as a human-readable UTC string.

 Example output:
	`Fri Jul 19 2025 at 21:47:30 UTC`

 @return 	A string containing the current date and time in UTC format.
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

// Checks if a character is a letter (`a-z`, `A-Z`)
// Returns true if the character is a letter, false otherwise.
bool	isLetter(char c)
{
	const std::string	letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	return letters.find(c) != std::string::npos;
}

// Checks if a character is a digit (`0-9`)
// Returns true if the character is a digit, false otherwise.
bool	isDigit(char c)
{
	const std::string	numbers = "0123456789";
	return numbers.find(c) != std::string::npos;
}

// Checks if a character is a special character
// (`-`, `[`, `]`, `\`, `` ` ``, `^`, `{`, `}`)
// Returns true if the character is a special character, false otherwise.
bool	isSpecial(char c)
{
	const std::string	specials = "-[]\\`^{}";
	return specials.find(c) != std::string::npos;
}
