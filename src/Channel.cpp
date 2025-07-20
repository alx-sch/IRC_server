#include "../include/Channel.hpp"

void Channel::add_user(const std::string& user_nick)
{
	if (_channel_members_by_nickname.find(user_nick) == _channel_members_by_nickname.end())
		_channel_members_by_nickname.insert(user_nick);
	_connected_user_number++;
}


void Channel::remove_user(const std::string& user_nick)
{
	if (_channel_members_by_nickname.find(user_nick) != _channel_members_by_nickname.end())
		_channel_members_by_nickname.erase(user_nick);

	_connected_user_number--;
}

void Channel::make_user_operator(const std::string& user_nick)
{
	if (_channel_operators_by_nickname.find(user_nick) == _channel_operators_by_nickname.end())
		_channel_operators_by_nickname.insert(user_nick);
	
}

void Channel::remove_user_operator_status(const std::string& user_nick)
{
	if (_channel_operators_by_nickname.find(user_nick) != _channel_operators_by_nickname.end())
		_channel_operators_by_nickname.erase(user_nick);
}

bool Channel::is_user_member(const std::string& user_nick)
{
	if (_channel_members_by_nickname.find(user_nick) != _channel_members_by_nickname.end())
		return true;
	return false;
}

bool Channel::is_user_operator(const std::string& user_nick)
{
	if (_channel_operators_by_nickname.find(user_nick) != _channel_operators_by_nickname.end())
		return true;
	return false;
}

bool Channel::can_user_join(const std::string &user_nick, const std::string &provided_key) const
{
	if (has_user_limit() && is_at_user_limit())
		return false;
	if (has_password() && !validate_password(provided_key))
		return false;
	if (is_invite_only() && !is_invited(user_nick))
		return false;
	return true;
}

void Channel::set_topic(const std::string &topic)
{
	_channel_topic = topic;
}

bool Channel::has_topic_protection() const
{
	return _topic_protection;
}

std::string Channel::get_topic() const
{
	return _channel_topic;
}

bool Channel::has_user_limit() const
{

	return (_user_limit > 0);
}

bool Channel::is_at_user_limit() const
{
	return (_connected_user_number == _user_limit);
}

void Channel::set_user_limit(const int new_limit) 
{
	_user_limit = new_limit;
}

int Channel::get_user_limit() const
{
	return _user_limit;
}

void Channel::set_invite_only()
{
	_invite_only = true;
}

bool Channel::is_invite_only() const
{
	return _invite_only;
}

bool Channel::is_invited(const std::string& nickname) const
{
	if (_channel_invitation_list.find(nickname) == _channel_invitation_list.end())
		return false;
	return true;
}

void Channel::add_invite(const std::string &nickname)
{
    if (_channel_invitation_list.find(nickname) == _channel_invitation_list.end())
        _channel_invitation_list.insert(nickname);
}

bool Channel::has_password() const
{
	if (!_channel_key.empty())
		return true;
	return false;
}

void Channel::set_password(const std::string &password)
{
	_channel_key = password;
}

bool Channel::validate_password(const std::string &password) const
{
	if( !has_password() || (password == _channel_key))
		return true;
	return false;
}

const std::string& Channel::get_name() const { return _channel_name; }

Channel::Channel(std::string name)
    : _channel_name(name), _connected_user_number(0), _user_limit(0),
        _invite_only(false), _topic_protection(false) {}
