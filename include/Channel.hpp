#include <set>
#include <string>

#ifndef CHANNEL_HPP
#define CHANNEL_HPP
class Channel
{
private:
	std::set<std::string> channel_members_by_nickname;
	std::set<std::string> channel_operators_by_nickname;
public:
	Channel(/* args */);
	~Channel();
	void add_user(std::string user_nick);
	void remove_user(std::string user_nick);
	void make_user_operator(std::string user_nick);
	void remove_user_operator_status(std::string user_nick);
	bool is_user_member(std::string user_nick);
	bool is_user_operator(std::string user_nick);
};

Channel::Channel(/* args */)
{
}

Channel::~Channel()
{
}

#endif
