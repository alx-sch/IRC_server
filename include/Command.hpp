#include <vector>
#include <string>

#ifndef COMMAND_HPP
#define COMMAND_HPP
class Command
{
private:
	enum class Type {
		JOIN,
		USER,
		KICK,
		INVITE,
		TOPIC,
		MODE_CHANNEL,
		MODE_NICK,
		LIST
	};
	std::vector<std::string> parameters;
public:
	Command(/* args */);
	~Command();
};

Command::Command(/* args */)
{
}

Command::~Command()
{
}

#endif