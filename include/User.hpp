#ifndef USER_HPP
# define USER_HPP

#include <string>

class	User
{
	private:
		int				_fd;
		std::string		_nickname;
		std::string		_username;

		// Disable default constructor / assignment operator
		User();
		User&	operator=(const User& other);

	public:
		User(int fd);
		User(const User& other);
		~User();

		void				setNickname(const std::string& nickname);
		void				setUsername(const std::string& username);

		int					getFd() const;
		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
};

#endif 
