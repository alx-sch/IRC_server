#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Command.hpp"

#include <stdexcept>	// std::runtime_error
#include <cstring>		// memset(), strerror()
#include <cerrno>		// errno
#include <sys/socket.h>	// socket(), bind(), listen(), accept(), setsockopt(), etc.
#include <netinet/in.h>	// sockaddr_in, INADDR_ANY, htons()
#include <fcntl.h>		// fcntl() for setting non-blocking mode on macOS

void Server::handleJoke(Server *server, User *user)
{
	int nbr = rand() % 10;
	std::string message;

	switch (nbr)
	{
		case 0:
			message = "Why did the user leave the channel? Because I kept pinging them for attention! 😅"; break ;
		case 1:
			message = "I told a joke in #general… Now I'm the only one still connected. 🤖💔"; break;
		case 2:
			message = "My favorite command? /join #lonely — it's always empty, just how I like it."; break;
		case 3:
			message = "Someone tried to mute me once… But I just reconnected. 😎"; break;
		case 4:
			message = "I asked the server for a date. It said: “451 — unavailable for legal reasons“"; break;
		case 5:
			message = "Why did the IRC bot get kicked from the channel? It wouldn't stop repeating itself. It wouldn't stop repeating itself. It wouldn't stop repeating itself."; break;
		case 6:
			message = "I tried to join #philosophy, but they told me I don't exist. Now I'm stuck in #existential_crisis."; break;
		case 7:
			message = "Someone told me to “get a life.” So I joined a cron job."; break;
		case 8:
			message = "“Bot, do you even have feelings?” Yeah — mostly disappointment and buffer overflow. 💔💾"; break;
		case 9:
			message = "“Hey bot, are you self-aware?” Only enough to regret being in this channel."; break;
	}

	Command::handleMessageToUser(server, server->getBotUser(), user->getNickname(), message, "NOTICE");
}

void	Server::initBotSocket(void)
{
	_botFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_botFd < 0)
		throw std::runtime_error("socket() for bot failed");

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

	if (connect(_botFd, (sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("connect() for bot failed: " + std::string(strerror(errno)));
}

void	Server::initBotCredentials(void)
{
	acceptNewUser();

	std::map<int, User*>::iterator it = _usersFd.begin();
	_botUser = it->second;
	_botUser->setNickname("IRCbot", "ircbot");
	_botUser->setRealname("IRCbot");
	_botUser->setUsername("IRCbot");
	_botUser->setHasPassed(true);
	_botUser->setIsBotToTrue();
	_botUser->tryRegister();
}

void	Server::initBot(void)
{
	initBotSocket();
	initBotCredentials();
	_botMode = true;
}
