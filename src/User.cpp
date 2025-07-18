
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

std::string&	User::getInputBuffer()
{
	return _inputBuffer;
}
