#ifndef UTILS_HPP
# define UTILS_HPP

# include <cstdlib>		// strtol
# include <ctime>		// time_t, gmtime, strftime
# include <iostream>
# include <iomanip>		// std::setw, std::left, std::right
# include <sstream>		// std::ostringstream
# include <stdexcept>	// std::runtime_error
# include <string>		// std::string

int			parsePort(const char* arg);
std::string	getFormattedTime();
std::string	getTimestamp();
bool		isValidNick(const std::string& nick);
bool		isValidChannelName(const std::string& channelName);
void		logUserAction(const std::string& nick, int fd, const std::string& message);
void		logServerMessage(const std::string& message);

// Converts any type to a `std::string` using stringstream
template <typename T>
std::string	toString(const T& value)
{
	std::ostringstream	oss;
	oss << value;
	return oss.str();
}

#endif
