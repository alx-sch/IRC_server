
#include <unistd.h>		// close()

#include "../include/User.hpp"

/////////////////////////////////
// Constructors and Destructor //
/////////////////////////////////

User::User(int fd)  : _fd(fd), _nickname(""), _username("") {}

User::User(const User& other) 
	: _fd(other._fd), _nickname(other._nickname), _username(other._username) {}

User::~User()
{
	if (_fd != -1) {
		close(_fd);
	}
}

/////////////////////////
// SETTERS AND GETTERS //
/////////////////////////

void	User::setNickname(const std::string& nickname)
{
	_nickname = nickname;
}

void	User::setUsername(const std::string& username)
{
	_username = username;
}

int	User::getFd() const
{
	return _fd;
}

const std::string&	User::getNickname() const
{
	return _nickname;
}

const std::string&	User::getUsername() const
{
	return _username;
}
