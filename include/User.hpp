// Using HexChat IRC client as a reference

#ifndef USER_HPP
# define USER_HPP

#include <string>
#include <vector>

class	User
{
	public:
		User();
		~User();
        User(int fd);

		void				setNickname(const std::string& nickname);
		void				setUsername(const std::string& username);
		void				setRealname(const std::string& realname);

		const int&			getFd() const;
		std::string&		getInputBuffer();
		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
		const std::string&	getRealname() const;


	private:
		// Disable constructor and copying (makes no sense for a user)
		User(const User& other);
		User&	operator=(const User& other);

		int							_fd;			// File descriptor (socket) for the user

		std::string					_nickname;		// required but must be unique
		std::string					_username;		// required
		std::string					_realname;		// required - usually unused

		std::string					_inputBuffer;	// buffer for incoming messages
		std::vector<std::string>	_opChannels;	// channels where this user has operator privileges
};

#endif
