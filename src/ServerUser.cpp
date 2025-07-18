#include <cerrno>		// errno
#include <cstring>		// strerror()
#include <iostream>
#include <stdexcept>	// std::runtime_error()
#include <unistd.h>		// close()

#include <arpa/inet.h>	// inet_ntoa()
#include <netinet/in.h>	// sockaddr_in, ntohs()

#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/defines.hpp"	// color formatting

/**
 Accepts a new user connection and adds them to the server's user lists
 (`_usersFd`, `_usersNick`) by creating a new `User` object.
*/
void	Server::acceptNewUser()
{
	int			userFd;		// fd for the accepted user connection
	sockaddr_in	userAddr;	// Init user address structure
	socklen_t	userLen = sizeof(userAddr);

	userFd = accept(_fd, reinterpret_cast<sockaddr*>(&userAddr), &userLen);
	if (userFd == -1)
		throw std::runtime_error("accept() failed: " + std::string(strerror(errno)));

	std::cout	<< "User "<< YELLOW << "#" << userFd << RESET << " connected from "
				<< YELLOW << inet_ntoa(userAddr.sin_addr)	// IP address as string
				<< ":" << ntohs(userAddr.sin_port)			// Port (convert from network order)
				<< RESET << std::endl;

	User* newUser = new User();
	_usersFd[userFd] = newUser;
}

/**
 Retrieves an `User` object by its file descriptor (fd) in a safe manner.

 This method safely searches the `_usersFd` map using `.find()` (instead of `[]`)
 to avoid accidental insertion of invalid keys.

 @param fd 	The file descriptor (socket) of the user to retrieve.
 @return 	Pointer to the `User` object if found, `NULL` otherwise.
*/
User*	Server::getUser(int fd) const
{
	std::map<int, User*>::const_iterator	it = _usersFd.find(fd);
	if (it != _usersFd.end())
		return it->second;
	return NULL;
}

/**
 Retrieves an `User` object by its nickname in a safe manner.

 This method safely searches the `_usersNick` map using `.find()` (instead of `[]`)
 to avoid accidental insertion of invalid keys.

 @param nickname 	The nickname of the user to retrieve.
 @return 			Pointer to the `User` object if found, `NULL` otherwise.
*/
User*	Server::getUser(const std::string& nickname) const
{
	std::map<std::string, User*>::const_iterator	it = _usersNick.find(nickname);
	if (it != _usersNick.end())
		return it->second;
	return NULL;
}

// Deletes a user from the server (`_usersFd`, `_usersNick`) using their file descriptor.
void	Server::deleteUser(int fd)
{
	User* user = getUser(fd);
	if (!user)
		return;

	close(fd);
	_usersFd.erase(fd);
	_usersNick.erase(user->getNickname());
	delete user;
}

// Deletes a user from the server (`_usersFd`, `_usersNick`) using their nickname.
void	Server::deleteUser(const std::string& nickname)
{
	User* user = getUser(nickname);
	if (!user)
		return;

	close(user->getFd());
	_usersNick.erase(nickname);
	_usersFd.erase(user->getFd());
	delete user;
}
