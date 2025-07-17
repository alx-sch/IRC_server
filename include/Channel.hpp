#include <set>
#include <string>

#ifndef CHANNEL_HPP
#define CHANNEL_HPP
class Channel
{
private:
	std::set<std::string> channel_members_by_nickname;
public:
	Channel(/* args */);
	~Channel();
};

Channel::Channel(/* args */)
{
}

Channel::~Channel()
{
}
#endif