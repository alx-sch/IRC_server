#include <set>
#include <string>

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
