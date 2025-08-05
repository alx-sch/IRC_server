#include <string>

#include "../include/Command.hpp"
#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Channel.hpp"
#include "../include/utils.hpp"

#include "../include/defines.hpp" // color formatting

// Command: PRIVMSG <recipient>{,<recipient>} :<text to be sent>
void	Command::handlePrivmsg(Server *server, User *user, const std::vector<std::string> &tokens)
{
	if (tokens.size() < 3)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent invalid PRIVMSG command (too few arguments)");
		user->replyError(411, "", "No recipient given (PRIVMSG)");
		return;
	}

	const std::string	&targetName = tokens[1];

  // Reconstruct the full message from tokens[2] onward
  std::string message;
  for (size_t i = 2; i < tokens.size(); ++i) {
    if (i > 2)
      message += " ";
    message += tokens[i];
  }

  // Remove leading ':' if present (trailing parameter)
  if (!message.empty() && message[0] == ':') {
    message = message.substr(1);
  }

  if (targetName[0] == '#') {
    // It's a channel
    Channel *channel = server->getChannel(targetName);
    if (!channel) {
      std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
                << "fd " << user->getFd() << RESET << ") "
                << "tried to PRIVMSG non-existing channel: " << targetName
                << "\n";
      user->replyError(403, targetName, "No such nick/channel");
      return;
    }

    if (!channel->is_user_member(user->getNickname())) {
      std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
                << "fd " << user->getFd() << RESET << ") "
                << "tried to PRIVMSG channel " << targetName
                << " but is not a member\n";
      user->replyError(404, targetName,
                       "Cannot send to channel (not a member)");
      return;
    }

    // Broadcast message to all channel members except sender
    std::string privmsgLine = ":" + user->getNickname() + " PRIVMSG " + targetName + " :" + message + "\r\n";
    broadcastToChannel(server, channel, privmsgLine, user->getNickname());

    std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
              << "fd " << user->getFd() << RESET << ") "
              << "sent PRIVMSG to channel " << targetName << ": " << message
              << "\n";
    return;
  } else {
    // It's a user
    User *targetUser = server->getUser(targetName);
    if (!targetUser) {
      std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
                << "fd " << user->getFd() << RESET << ") "
                << "tried to PRIVMSG non-existing user: " << targetName << "\n";
      user->replyError(401, targetName, "No such nick/channel");
      return;
    }

    // Send private message to target user
    std::string privmsgLine = ":" + user->getNickname() + " PRIVMSG " +
                              targetName + " :" + message + "\r\n";
    targetUser->getOutputBuffer() += privmsgLine;

    std::cout << GREEN << user->getNickname() << RESET << " (" << MAGENTA
              << "fd " << user->getFd() << RESET << ") "
              << "sent PRIVMSG to user " << targetName << ": " << message
              << "\n";
  }
}

void	Command::handleNotice(Server* server, User* user, const std::vector<std::string>& tokens)
{
	(void)server;
	(void)user;
	(void)tokens;
}