#include "../include/Channel.hpp"

void Channel::add_user(std::string user_nick)
{
	if (channel_members_by_nickname.find(user_nick) == channel_members_by_nickname.end())
		channel_members_by_nickname.insert(user_nick);
}


void Channel::remove_user(std::string user_nick)
{
	if (channel_members_by_nickname.find(user_nick) != channel_members_by_nickname.end())
		channel_members_by_nickname.erase(user_nick);
}

void Channel::make_user_operator(std::string user_nick)
{
	if (channel_operators_by_nickname.find(user_nick) == channel_operators_by_nickname.end())
		channel_operators_by_nickname.insert(user_nick);
	
}

void Channel::remove_user_operator_status(std::string user_nick)
{
	if (channel_operators_by_nickname.find(user_nick) != channel_operators_by_nickname.end())
		channel_operators_by_nickname.erase(user_nick);
}

bool Channel::is_user_member(std::string user_nick)
{
	if (channel_members_by_nickname.find(user_nick) != channel_members_by_nickname.end())
		return true;
	return false;
}

bool Channel::is_user_operator(std::string user_nick)
{
	if (channel_operators_by_nickname.find(user_nick) != channel_operators_by_nickname.end())
		return true;
	return false;
}
