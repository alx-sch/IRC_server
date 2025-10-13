#ifndef DEFINES_HPP
# define DEFINES_HPP

# define SERVER_NAME		"42ircRebels.net"
# define NETWORK			"42 IRC"
# define VERSION			"eval-42.42"

# define LOG_RAW_CMDS		0	// '1': Commands as sent by users are logged; '0': not logged

# define BOT_NAME			"IRCbot"
# define BOT_COLOR			"\033[38;5;214m"	// Orange color for bot messages
# define BOT_SILENT_NOTE	1	// '1': No logging of bot NOTICE messages; '0': log them

# define MAX_CHANNELS		10		// Max channels per user; recommended in RFC 1459, 1.3
# define C_MODES			"itkol"	// Supported channel modes, as per subject
# define U_MODES			"-"		// No user modes implemented

// Below is all according to RFC 1459:

# define MAX_BUFFER_SIZE	512		// You can send longer messages, 'recv' just reads in 512-byte chunks.
# define MAX_NICK_LENGTH	9		// according to RFC 1459, 1.2
# define MAX_CHANNEL_LENGTH	24		// according to RFC 1459, 1.3 that's max. 200; but we can use less

# define RED				"\033[31m"			// used for errors / invalid input
# define GREEN				"\033[32m"			// used for nicknames
# define YELLOW				"\033[33m"			// used for user settings
# define MAGENTA			"\033[35m"			// used for fds
# define CYAN				"\033[36m"			// used for timestamps
# define BLUE				"\033[34m"			// used for channels
# define BOLD				"\033[1m"
# define RESET				"\033[0m"

#endif
