#ifndef UTILS_HPP
# define UTILS_HPP

# include <cstdlib>		// strtol
# include <sstream>		// std::ostringstream
# include <stdexcept>	// std::runtime_error
# include <string>		// std::string

int			parsePort(const char* arg);
std::string	toString(int value);

#endif
