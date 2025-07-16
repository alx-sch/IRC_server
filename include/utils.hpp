#ifndef UTILS_HPP
# define UTILS_HPP

# include <cstdlib>		// strtol
# include <string>		// std::string
# include <stdexcept>	// std::runtime_error
# include <sstream>		// std::ostringstream

int			parsePort(const char* arg);
std::string	toString(int value);

#endif
