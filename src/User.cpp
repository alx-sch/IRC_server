
#include "../include/User.hpp"

User::User() {}
User::~User() {}

void	User::setNickname(const std::string& nickname)
{
	_nickname = nickname;
}

void	User::setUsername(const std::string& username)
{
	_username = username;
}

void	User::setRealname(const std::string& realname)
{
	_realname = realname;
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

const std::string&	User::getRealname() const
{
	return _realname;
}

std::string&	User::getInputBuffer()
{
	return _inputBuffer;
}
