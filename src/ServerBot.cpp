#include "../include/Server.hpp"
#include "../include/User.hpp"
#include "../include/Command.hpp"
#include "../include/utils.hpp"
#include "../include/defines.hpp"	// BOT_NAME

#include <stdexcept>	// std::runtime_error
#include <cstring>		// memset(), strerror()
#include <cerrno>		// errno
#include <sys/socket.h>	// socket(), bind(), listen(), accept(), setsockopt(), etc.
#include <netinet/in.h>	// sockaddr_in, INADDR_ANY, htons()
#include <fcntl.h>		// fcntl() for setting non-blocking mode on macOS
#include <stack>

//////////////////
// Bot commands //
//////////////////

static long	evaluateExpression(const std::string &expr);
static bool	isValidExpression(const std::string &expr);

/**
Handles the custom IRCbot `CALC` command, evaluating a simple math expression and replying with the result.

This function parses and computes a basic arithmetic expression provided by the user.
Supported operators: `+`, `-`, `*`, `/`
Only digits and the mentioned operators are supported.

Syntax:
	CALC <expression>

Example:
	CALC 3+5*2

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the CALC command.
 @param tokens	Vector containing the command and expression tokens.
*/
void	Server::handleCalc(Server *server, User *user, const std::vector<std::string>& tokens)
{
	if (!Command::checkRegistered(user, "CALC"))
		return;

	// Needs 2 tokens: CALC <expression>
	if (tokens.size() < 2)
	{
		logUserAction(user->getNickname(), user->getFd(), "sent CALC without a math expression");
		user->sendError(461, "CALC", "Not enough parameters");
		return;
	}

	std::string	expression = tokens[1];
	for (size_t i = 2; i < tokens.size(); ++i)
		expression += tokens[i]; // Concatenate all tokens to form the full expression

	if (!isValidExpression(expression))
	{
		logUserAction(user->getNickname(), user->getFd(), "sent CALC with an invalid math expression");
		user->sendError(461, "CALC", "Invalid expression. Only digits and operators (+-*/) are allowed.");
		return;
	}

	long	result = 0;
	try
	{
		result = evaluateExpression(expression);
	}
	catch (const std::runtime_error &e)
	{
		user->sendError(400, "CALC", e.what()); // Division by zero
		return;
	}

	// Convert result to string
	std::ostringstream oss;
	oss << result;
	std::string resultStr = oss.str();

	// Send result as a NOTICE from the bot
	Command::handleMessageToUser(server, server->getBotUser(), user->getNickname(),
		"The answer to " + expression + " is: " + resultStr, "NOTICE");
}

/**
Helper function for handleCalc().
The function evaluates a simple integer arithmetic expression containing digits and the operators + - * / (no parentheses).
It scans the string left→right, builds multi-digit numbers, and uses a stack to enforce * and / having higher precedence than + and -. 
A sentinel character at the end forces the last number to be processed.
Division by zero throws a std::runtime_error.
*/
static long	evaluateExpression(const std::string &expr)
{
	std::stack<long>	numbers; // Holds intermediate/temporary values that will later be summed.
	long				currentNumber = 0; // Accumulates digits into an integer.
	char				lastOperator = '+'; // Remembers the operator that applies to currentNumber. It starts as '+' so the first number is pushed as positive.

	for (size_t i = 0; i <= expr.size(); ++i)
	{
		// '\0' acts as a sentinel character — it’s not part of the expression, but it forces the code to process the last number before exiting the loop.
		char	c = (i < expr.size()) ? expr[i] : '\0';

		if (isdigit(c))
			currentNumber = currentNumber * 10 + (c - '0'); // If digit, store the current digit in currentNumber by its integer value.

		// If an operator or end of string
		if (!isdigit(c)|| i == expr.size())
		{
			long	top = numbers.empty() ? 0 : numbers.top(); // Reads the current top of the stack (or 0 if empty).

			if (lastOperator == '+')
				numbers.push(currentNumber); // Push currentNumber (deferred addition - it will happen later).
			else if (lastOperator == '-')
				numbers.push(-currentNumber); // Push -currentNumber (deferred subtraction - it will happen later).
			else if (lastOperator == '*') 
			{
				numbers.pop(); // Pop the previous value top
				numbers.push(top * currentNumber); // Multiply top * currentNumber, and push result (immediate multiplication enforces precedence).
			}
			else if (lastOperator == '/') // Same as * but with integer division; checks currentNumber == 0 and throws on divide-by-zero.
			{
				if (currentNumber == 0)
					throw std::runtime_error("Division by zero");
				numbers.pop();
				numbers.push(top / currentNumber);
			}

			lastOperator = c; // After processing, set lastOperator to the operator we just read.
			currentNumber = 0; // Reset currentNumber to 0.
		}
	}

	long	result = 0; // All deferred additions/subtractions are accumulated to produce the final integer result.
	while (!numbers.empty())
	{
		result += numbers.top();
		numbers.pop();
	}

	return result;
}

/**
Helper function for handleCalc().
Validates math expression (it can only contain digits, '+', '-', '*' and '/').

 @param expr	Math expression (string).

 @return		True if valid, false if invalid.
*/
static bool	isValidExpression(const std::string &expr)
{
	for (size_t i = 0; i < expr.size(); ++i)
	{
		char c = expr[i];
		if (!isdigit(c) && c != '+' && c != '-' && c != '*' && c != '/')
			return false;
	}
	return true;
}

/**
Handles the custom IRCbot `JOKE` command, sending a `NOTICE` to the user with a stupid joke.

This function sends a random joke (10 possible outcomes).

Syntax:
	JOKE

 @param server	Pointer to the server instance handling the command.
 @param user	The user issuing the JOKE command.
*/
void	Server::handleJoke(Server *server, User *user)
{
	if (!Command::checkRegistered(user, "JOKE"))
		return ;

	int			nbr = rand() % 10;
	std::string	message;

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


//////////////////////
// Initializing Bot //
//////////////////////

/**
Initializes and connects the internal bot socket.

This function creates a new TCP socket dedicated to the server’s built-in bot.
The bot connects to the same IRC server instance via the local loopback interface
(127.0.0.1) using the server’s own listening port.

Steps:
 - 1. Create a socket using AF_INET (IPv4) and SOCK_STREAM (TCP).
 - 2. Configure the socket address to point to localhost and the server port.
 - 3. Attempt to connect the bot socket to the server.
*/
void	Server::initBotSocket(void)
{
	_botFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_botFd < 0)
		throw std::runtime_error("socket() for bot failed");

	sockaddr_in	addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

	if (connect(_botFd, (sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("connect() for bot failed: " + std::string(strerror(errno)));
}

/*Accepts the bot user connection and adds it to the server's user lists
(_usersFd, _usersNick) by creating a new User object. Also sets the bot credentials and registers it.*/
void	Server::initBotCredentials(void)
{
	std::string	botName = BOT_NAME;

	acceptNewUser();

	// As the bot is the first user, it will be the first in the map.
	std::map<int, User*>::iterator	it = _usersFd.begin();
	_botUser = it->second;
	_botUser->setNickname(botName, normalize(botName));
	_botUser->setRealname(botName);
	_botUser->setUsername(botName);
	_botUser->setHasPassed(true);
	_botUser->setIsBotToTrue();
	_botUser->tryRegister();
}

/*Initializes bot socket and connects to server socket, sets bot credentials, and 
registers the bot user. Also sets _botMode to true.*/
void	Server::initBot(void)
{
	initBotSocket();
	initBotCredentials();
	_botMode = true;
}
