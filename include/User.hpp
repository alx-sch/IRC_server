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

		void				setNickname(const std::string& nickname);
		void				setUsername(const std::string& username);
		void				setRealname(const std::string& realname);
		void				setHost(const std::string& host);
		void				markDisconnected();

		const int&			getFd() const;
		std::string&		getInputBuffer();
		std::string&		getOutputBuffer();
		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
		const std::string&	getRealname() const;
		const std::string&	getHost() const;

		const std::set<std::string>&	getChannels() const;
		void				addChannel(const std::string& channel);
		void				removeChannel(const std::string& channel);

		bool				hasSendErrorLogged() const;
		void				setSendErrorLogged(bool value);

		// === UserReply.cpp ===

		void				replyWelcome();
		void				replyError(int code, const std::string& param,
								const std::string& message);
		void				replyServerMsg(const std::string& message);

		// === UserRegistration.cpp ===

		void				setHasPassed(bool b);	// Used in handlePass() command handler
		bool				isRegistered() const;
		void				tryRegister();

	private:
		// Disable constructor and copying (makes no sense for a user)
		User();
		User(const User& other);
		User&	operator=(const User& other);

		int							_fd;			// File descriptor (socket) for the user

		std::string					_nickname;
		std::string					_username;
		std::string					_realname;		// usually unused
		std::string					_host;			// rather obsolete, most clients use '*' -> but get from USER command nevertheless

		Server*						_server;		// Pointer to the server user is connected to (to use 'Server' methods)
		std::string					_inputBuffer;	// buffer for incoming messages (client->server), accumulated until a full message is formed
		std::string					_outputBuffer;	// buffer for outgoing messages (server->client), to be sent when socket is ready
		std::vector<std::string>	_opChannels;	// channels where this user has operator privileges
		std::set<std::string>		_channels;		// channels where this user is in
		bool						_hasNick;		// true if user has sent NICK command (got nickname)
		bool						_hasUser;		// true if user has sent USER command (got username)
		bool						_hasPassed;		// true if user has sent PASS command successfully
		bool						_isRegistered;	// true if user has sent NICK, USER commands to server

		bool						_sendErrorLogged;	// Flag to prevent multiple error logs for the same send error
};

#endif
