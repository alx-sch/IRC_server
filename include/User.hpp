#ifndef USER_HPP
# define USER_HPP

#include <string>

class	User
{
	private:
		// Disable default constructor and copying (makes no sense for a user)
		User();
		User(const User& other);
		User&	operator=(const User& other);

		int				_fd;		// fd for user's socket
		std::string		_nickname;
		std::string		_username;

	public:
		User(int fd);
		~User();

		void				setNickname(const std::string& nickname);
		void				setUsername(const std::string& username);

		int					getFd() const;
		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
};

#endif 
