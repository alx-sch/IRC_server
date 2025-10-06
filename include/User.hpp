// Using HexChat IRC client as a reference

#ifndef USER_HPP
# define USER_HPP

#include <set>
#include <string>
#include <vector>

class	Server;

class	User
{
	public:
		User(int fd, Server* server);
		~User();

		std::string			buildHostmask() const;

		void				setNickname(const std::string& displayNick, const std::string& canonicalNick);
		void				setUsername(const std::string& username);
		void				setUsernameTemp(const std::string& username);
		void				setRealname(const std::string& realname);
		void				setHost(const std::string& host);
		void				markDisconnected();
		void				setIsBotToTrue(void);

		int					getFd() const;
		std::string&		getInputBuffer();
		std::string&		getOutputBuffer();
		const std::string&	getNickname() const;
		const std::string&  getNicknameLower() const;
		const std::string&	getUsername() const;
		const std::string&	getRealname() const;
		const std::string&	getHost() const;
		const Server*		getServer() const;
		bool				getIsBot() const;

		const std::set<std::string>&	getChannels() const;
		void				addChannel(const std::string& channel);
		void				removeChannel(const std::string& channel);

		// === UserMessaging.cpp ===

		void				sendWelcome();
		void				sendError(int code, const std::string& param, const std::string& message);
		void				sendServerMsg(const std::string& message);
		void				sendMsgFromUser(const User* sender, const std::string& message);

		// === UserRegistration.cpp ===

		void				setHasPassed(bool b);
		bool				isRegistered() const;
		void				tryRegister();

	private:
		// Disable constructor and copying (makes no sense for a user)
		User();
		User(const User& other);
		User&	operator=(const User& other);

		int							_fd;			// File descriptor (socket) for the user

		std::string					_nickname;		// User's display nickname (as set by NICK command)
		std::string					_nicknameLower;	// Lowercase version for case-insensitive comparisons
		std::string					_username;
		bool						_hasUsername;	// true if username was set via USER command
		std::string					_realname;		// usually unused
		std::string					_host;			// rather obsolete, most clients use '*' -> use IP address obtained from socket

		Server*						_server;		// Pointer to the server user is connected to (to use 'Server' methods)
		std::string					_inputBuffer;	// buffer for incoming messages (client->server), accumulated until a full message is formed
		std::string					_outputBuffer;	// buffer for outgoing messages (server->client), to be sent when socket is ready
		std::vector<std::string>	_opChannels;	// channels where this user has operator privileges
		std::set<std::string>		_channels;		// channels where this user is in
		bool						_hasNick;		// true if user has sent NICK command (got nickname)
		bool						_hasUser;		// true if user has sent USER command (got username)
		bool						_hasPassed;		// true if user has sent PASS command successfully
		bool						_isRegistered;	// true if user has sent NICK, USER commands to server

		bool						_isBot; // true if user is IRCbot
};

#endif
