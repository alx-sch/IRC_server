#include "../include/utils.hpp"
#include "../include/Server.hpp"

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

// Converts an `int` to a `std::string`
std::string	toString(int value)
{
	std::ostringstream	oss;
	
	oss << value;
	return oss.str();
}
