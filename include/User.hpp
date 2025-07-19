// Using HexChat IRC client as a reference

#ifndef USER_HPP
# define USER_HPP

#include <string>
#include <vector>

class	Server;

class	User
{
	public:
		User(Server* server);
		~User();

		void				replyWelcome();
		void				replyError(int code, const std::string& message);
		void				markDisconnected(); // Sets _fd to -1 after deletingg from server

		void				setNickname(const std::string& nickname);
		void				setUsername(const std::string& username);
		void				setRealname(const std::string& realname);

		const int&			getFd() const;
		std::string&		getInputBuffer();
		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
		const std::string&	getRealname() const;
		bool				isRegistered() const;


	private:
		// Disable constructor and copying (makes no sense for a user)
		User();
		User(const User& other);
		User&	operator=(const User& other);

		int							_fd;			// File descriptor (socket) for the user

		std::string					_nickname;		// required but must be unique
		std::string					_username;		// required
		std::string					_realname;		// required - usually unused
		std::string					_host;			// Hostname of the user (optional, can be empty)
		bool 						_isRegistered;	// true if user has sent NICK, USER commands to server

		Server*						_server;		// Pointer to the server user is connected to (to use 'Server' methods)
		std::string					_inputBuffer;	// buffer for incoming messages
		std::vector<std::string>	_opChannels;	// channels where this user has operator privileges

		void						sendReply(const std::string& message);
};

#endif
