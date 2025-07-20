#ifndef UTILS_HPP
# define UTILS_HPP

# include <cstdlib>		// strtol
# include <ctime>		// time_t, gmtime, strftime
# include <sstream>		// std::ostringstream
# include <stdexcept>	// std::runtime_error
# include <string>		// std::string

int			parsePort(const char* arg);
std::string	getFormattedTime();
bool		isLetter(char c);
bool		isDigit(char c);
bool		isSpecial(char c);

// Converts any type to a `std::string` using stringstream
template <typename T>
std::string	toString(const T& value)
{
	std::ostringstream	oss;
	oss << value;
	return oss.str();
}

#endif
