#ifndef DEFINES_HPP
# define DEFINES_HPP

# define SERVER_NAME		"42ircRebels"
# define NETWORK			"42 IRC"
# define VERSION			"eval-42.42"
# define C_MODES			"abc"	// Channel modes
# define U_MODES			"xyz"	// User modes

// Below is all according to RFC 1459:

# define MAX_BUFFER_SIZE	512 // You can send longer messages, 'recv' just reads in 512-byte chunks.
# define MAX_NICK_LENGTH	9

# define RED				"\033[31m"
# define GREEN				"\033[32m"
# define YELLOW				"\033[33m"
# define MAGENTA			"\033[35m"
# define CYAN				"\033[36m"
# define BOLD				"\033[1m"
# define RESET				"\033[0m"

#endif
