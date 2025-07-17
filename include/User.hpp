#ifndef USER_HPP
# define USER_HPP

#include <string>

class	User
{
	private:
		// Disable constructor and copying (makes no sense for a user)
		User(const User& other);
		User&	operator=(const User& other);

		std::string		_nickname;
		std::string		_username;

	public:
		User();
		~User();

		void				setNickname(const std::string& nickname);
		void				setUsername(const std::string& username);

		const std::string&	getNickname() const;
		const std::string&	getUsername() const;
};

#endif 
