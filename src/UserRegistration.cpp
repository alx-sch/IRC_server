#include <iostream>

#include "../include/User.hpp"
#include "../include/utils.hpp"	// logUserAction()

// Sets status of whether the user has passed the password check to `b`.
void	User::setHasPassed(bool b)
{
	_hasPassed = b;
}

// Returns `true` if the user has been successfully registered.
bool	User::isRegistered() const
{
	return _isRegistered;
}

// Attempts to register the user if they have sent `NICK`, `USER`,
// and `PASS` commands. If successful, sends welcome messages to the user.
void	User::tryRegister()
{
	if (_isRegistered)
		return;	// Already registered -> do nothing

	if (_hasNick && _hasUser && _hasPassed)
	{
		_isRegistered = true;
		logUserAction("successfully registered", _isBot);
		sendWelcome();	// Send welcome messages
	}
}
