
#include <unistd.h>	// close()
#include "../include/User.hpp"

/////////////////////////////////
// Constructors and Destructor //
/////////////////////////////////

User::User() : _nickname(""), _username("") {}

User::~User() {}

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

const int&	User::getFd() const
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
