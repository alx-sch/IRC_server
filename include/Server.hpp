#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <vector>
# include <sys/select.h>	// for fd_set

class	User;	// no include needed as only pointer is used
class	Channel;

class Server
{
	public:
		// === Server.cpp ===

		Server(int port, const std::string& password);
		~Server();

		void	run();

		const std::string&	getServerName() const;
		const std::string&	getVersion() const;
		const std::string&	getNetwork() const;
		const std::string&	getCreationTime() const;
		const std::string&	getPassword() const;
		int					getPort() const;
		const std::string&	getCModes() const;
		const std::string&	getUModes() const;
		int					getMaxChannels() const;

		std::map<std::string, User*>&	getNickMap();
		void				removeNickMapping(const std::string& nickname);

		// === ServerUser.cpp ===

		User*		getUser(int fd) const;
		User*		getUser(const std::string& nickname) const;
		void		deleteUser(int fd, std::string logMsg);

		// === ServerChannel.cpp ===

		Channel*	getChannel(const std::string& channelName) const;
		Channel*	getOrCreateChannel(const std::string& channelName, User* user,
						const std::string& key = "", bool* wasCreated = NULL);
		void		deleteChannel(const std::string& channelName, std::string reason);

	private:
		// Disable default constructor and copying (makes no sense for a server)
		Server();
		Server(const Server& other); 
		Server&	operator=(const Server& other);

		const std::string		_name;		// Server name, used in replies
		const std::string		_version;	// Server version, used in replies
		const std::string		_network;	// Network name, used in replies
		const std::string		_creationTime;	// Server creation time, used in replies
		const int				_port;		// Server port
		const std::string		_password;	// Server password for client authentication

		int						_fd;		// server socket fd (listening socket)
		std::map<int, User*>	_usersFd;	// Keep track of active users by fd
		std::map<std::string, User*>	_usersNick;	// Keep track of active users by nickname

		std::map<std::string, Channel*>	_channels;	// Keep track of channels by name
		const std::string		_cModes;	// Channel modes, used in replies
		const std::string		_uModes;	// User modes, used in replies
		const int				_maxChannels;	// Max channels per user

		// === ServerSocket.cpp ===

		void		initSocket();
		void		createSocket();
		void		setSocketOptions();
		void		bindSocket();
		void		startListening();
		int			prepareReadSet(fd_set& readFds);
		int			prepareWriteSet(fd_set& writeFds);

		// === ServerUser.cpp ===

		void		acceptNewUser();
		void		handleReadyUsers(fd_set& readFds);
		void		handleWriteReadyUsers(fd_set& writeFds);
		void		handleSendError(int fd, const std::string& nick);
		bool		handleUserInput(int fd);
		std::vector<std::string>	extractMessagesFromBuffer(User* user);
		void		handleDisconnection(int fd, const std::string& reason, const std::string& source);
};

# endif
