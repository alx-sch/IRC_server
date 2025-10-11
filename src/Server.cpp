#include <iostream>
#include <string>
#include <stdexcept>	// std::runtime_error
#include <cerrno>		// errno
#include <cstring>		// memset(), strerror()
#include <ctime> 		// time()
#include <cstdlib>		// for srand(), rand()
#include <unistd.h>		// close()
#include <fstream>		// std::ofstream, open()
#include <iomanip>		// std::setw
#include <sys/select.h>	// select(), fd_set, FD_* macros

#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/defines.hpp"	// color formatting
#include "../include/signal.hpp"	// g_running variable
#include "../include/utils.hpp"		// getFormattedTime(), getTimestamp(), removeColorCodes()

/// Constructor: Initializes the server socket and sets up the server state.
Server::Server(int port, const std::string& password) 
	:	_name(SERVER_NAME), _version(VERSION), _network(NETWORK),
		_creationTime(getFormattedTime()), _port(port),
		_password(password), _fd(-1), _cModes(C_MODES), _uModes(U_MODES),
		_maxChannels(MAX_CHANNELS), _botMode(false), _botFd(-1), _botUser(NULL)
{
	initSocket();
	srand(time(0));
}

// Destructor: Closes the server socket and cleans up resources.
// Invoked when server.run() has exited due to SIGINT (Ctrl+C)
Server::~Server()
{
	// Close the listening socket if open
	if (_fd != -1)
		close(_fd);

	if (_botFd != -1)
		close(_botFd);
	
	std::cout << std::endl;	// Just a newline for clean output after Ctrl+C
	logServerMessage("Shutting down server...");

	// Delete all dynamically allocated User objects
	while (!_usersFd.empty())
		deleteUser(_usersFd.begin()->first, toString("disconnected (") + YELLOW + "server shutdown" + RESET + ")");

	// Delete all dynamically allocated Channel objects
	while (!_channels.empty())
		deleteChannel(_channels.begin()->first, "server shutdown");

	logServerMessage("Server shutdown complete");

	if (_logFile.is_open())
	{
		logServerMessage(toString("Log file closed: ") + YELLOW + _logFilePath + RESET);
		_logFile.close();
	}

}

/**
Starts the main server loop to handle incoming connections and client messages.

Sets up the `fd_set` for `select()`, and continuously monitors:
 - The listening socket for new client connections.
 - All active user sockets for incoming messages.

The loop runs until interrupted by `SIGINT` (Ctrl+C), at which point `g_running` becomes 0.
*/
void	Server::run()
{
	fd_set	readFds, writeFds;	// Sets of fds to monitor for readability and writability
	int		maxFd;		// Highest fd in the set, used by select() to avoid scanning all fds
	int		writeMaxFd;	// Highest fd in the write set
	int		ready;		// Number of ready fds returned by select()

	openLogFile();
	logServerMessage(toString("Server running on port ") + YELLOW + toString(getPort()) + RESET);

	// Initializes the bot if bot mode is set.
	#ifdef BOT_MODE
		initBot();
	#endif

	while (g_running)
	{
		maxFd = prepareReadSet(readFds);
		writeMaxFd = prepareWriteSet(writeFds);
		if (writeMaxFd > maxFd) maxFd = writeMaxFd;

		// Pause the program until a socket becomes readable or writable imn any of the provided sets
		// 'exceptional' set is not used (NULL), also no timeour set (NULL)
		ready = select(maxFd + 1, &readFds, &writeFds, NULL, NULL);
		if (ready == -1) // Critical! Shut down server / end program
		{
			if (errno == EINTR) // If interrupted by signal (SIGINT), just return to main.
				return;
			throw std::runtime_error("select() failed: " + toString(strerror(errno)));
		}

		// New incoming connection?
		if (FD_ISSET(_fd, &readFds)) // checks if server socket (_fd) is ready for reading -> new connection
			acceptNewUser(); // Adds user to `_usersFd`

		// Handle user input for all active connections (messages, disconnections)
		handleReadReadyUsers(readFds);
		
		// Handle pending output to be sent to users
		handleWriteReadyUsers(writeFds);
	}
}

/////////////
// LOGGING //
/////////////

// Logs a general server message with timestamp.
void	Server::logServerMessage(const std::string& message)
{
	std::string			timeStamp = getTimestamp();
	std::ostringstream	fileLogEntry;

	// log into console
	std::cout	<< "[" << CYAN << timeStamp << RESET << "] "
				<< std::left << std::setw(MAX_NICK_LENGTH + 10) << " " // pad for alignment with user logs
				<< message << std::endl;

	// log into file
	fileLogEntry	<< "[" << timeStamp << "] "
					<< std::left << std::setw(MAX_NICK_LENGTH + 10) << " " 
					<< removeColorCodes(message) << "\n";

	if (_logFile.is_open())
	{
		_logFile << fileLogEntry.str();
		_logFile.flush(); // Ensure the data is written immediately to disk
	}
}

/**
Returns the current UTC formatted as a readable string.
Used in filename for server logging (YYYYMMDD_HHMMSS).

Example output:
	`20250803_184739`

 @return	A string containing the current date and time in UTC format.
*/
static std::string	getFileTimestamp()
{
	std::time_t	now = std::time(NULL);		// Get current time as time_t (seconds since epoch)
	std::tm*	gmt = std::gmtime(&now);	// Convert to UTC time (struct tm)
	char		buffer[128];

	// Format: YYYYMMDD_HHMMSS
	std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", gmt);

	return std::string(buffer);
}

void	Server::openLogFile()
{
	_logFilePath = _name + "_" + getFileTimestamp() + ".log";

	_logFile.open(_logFilePath.c_str(), std::ios::out | std::ios::app);
	if (!_logFile.is_open())
		throw std::runtime_error(toString("Failed to open log file: ") + YELLOW + _logFilePath + RESET);
	logServerMessage(toString("Log file opened: ") + YELLOW + _logFilePath + RESET);
}

/////////////
// Getters //
/////////////

// Returns the server name.
const std::string&	Server::getServerName() const
{
	return _name;
}

// Returns the server version.
const std::string&	Server::getVersion() const
{
	return _version;
}

// Returns the network name.
const std::string&	Server::getNetwork() const
{
	return _network;
}

// Returns the server creation time.
const std::string&	Server::getCreationTime() const
{
	return _creationTime;
}

// Returns the server password.
const std::string&	Server::getPassword() const
{
	return _password;
}

// Returns the server port.
int	Server::getPort() const
{
	return _port;
}

// Returns the channel modes string.
const std::string&	Server::getCModes() const
{
	return _cModes;
}

// Returns the user modes string.
const std::string&	Server::getUModes() const
{
	return _uModes;
}

// Returns the maximum number of channels a user can join.
int	Server::getMaxChannels() const
{
	return _maxChannels;
}

// True if program is running in bot mode, as defined in Makefile.
bool	Server::getBotMode() const
{
	return _botMode;
}

// Returns the IRC bot user instance.
User*	Server::getBotUser() const
{
	return _botUser;
}

// Returns the log file output stream.
std::ofstream&	Server::getLogFile()
{
	return _logFile;
}

//////////////////
// Nick Mapping //
//////////////////

// Returns a map of active users by nickname.
std::map<std::string, User*>&	Server::getNickMap()
{
	return _usersNick;
}

// Removes a nickname mapping from the server's user map.
// Used when user changes their nickname.
void	Server::removeNickMapping(const std::string& nickname)
{
	_usersNick.erase(nickname);
}
