#ifndef UTILS_HPP
# define UTILS_HPP

# include <sstream>	// std::ostringstream
# include <string>	// std::string

int			parsePort(const char* arg);
std::string	getFormattedTime();
std::string	getTimestamp();
bool		isValidNick(const std::string& nick);
bool		isValidChannelName(const std::string& channelName);
void		logUserAction(const std::string& nick, int fd, const std::string& message, bool botMode = false);
std::string	normalize(const std::string& name);
std::string	removeColorCodes(const std::string& str);

// Converts any type to a `std::string` using stringstream
template <typename T>
std::string	toString(const T& value)
{
	std::ostringstream	oss;
	oss << value;
	return oss.str();
}

#endif
