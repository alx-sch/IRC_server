#include "../include/User.hpp"

User::User(Server* server)
	: _fd(-1), _isRegistered(false), _server(server) {}

User::~User() {}

/////////////
// Setters //
/////////////

// This sets the file descriptor to -1, indicating the user is no longer connected.
void	User::markDisconnected()
{
	_fd = -1;
}

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

/////////////
// Getters //
/////////////

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

bool	User::isRegistered() const
{
	return _isRegistered;
}
