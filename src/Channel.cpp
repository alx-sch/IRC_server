#include "../include/Channel.hpp"

// Constructor: Initializes the channel with a name and default values.
Channel::Channel(std::string name)
	:	_channel_name(name), _connected_user_number(0), _user_limit(0),
		_invite_only(false), _topic_protection(false)
{}

// Default destructor
Channel::~Channel() {}

// Adds a user to the channel and increments member count if successful.
void	Channel::add_user(const std::string& user_nick)
{
	bool	inserted = _channel_members_by_nickname.insert(user_nick).second;
	if (inserted)
		++_connected_user_number;
}

// Removes a user from the channel and decrements member count if they were present.
void	Channel::remove_user(const std::string& user_nick)
{
	bool	removed = _channel_members_by_nickname.erase(user_nick);
	if (removed)
		--_connected_user_number;
}

// Grants operator status to the given user.
void	Channel::make_user_operator(const std::string& user_nick)
{
	_channel_operators_by_nickname.insert(user_nick);
}

// Revokes operator status from the given user.
void	Channel::remove_user_operator_status(const std::string& user_nick)
{
	_channel_operators_by_nickname.erase(user_nick);
}

// Checks whether the given user is a channel member.
bool	Channel::is_user_member(const std::string& user_nick) const
{
	return (_channel_members_by_nickname.count(user_nick) > 0);
}

// Checks whether the given user is a channel operator.
bool	Channel::is_user_operator(const std::string& user_nick) const
{
	return (_channel_operators_by_nickname.count(user_nick) > 0);
}

// Checks if a user can join the channel with the provided key.
// Overwrites `result` with the appropriate JoinResult enum value.
bool	Channel::can_user_join(const std::string &user_nick, const std::string &provided_key,
	JoinResult& result) const
{
	if (has_user_limit() && is_at_user_limit())
	{
		result = JOIN_FULL;
		return false;
	}
	if (has_password() && !validate_password(provided_key))
	{
		result = JOIN_BAD_KEY;
		return false;
	}	
	if (is_invite_only() && !is_invited(user_nick))
	{
		result = JOIN_INVITE_ONLY;
		return false;
	}
	return true;
}

// Returns true if topic protection is enabled.
bool Channel::has_topic_protection() const
{
	return _topic_protection;
}

// Sets the topic of the channel.
void Channel::set_topic(const std::string &topic)
{
	_channel_topic = topic;
}

// Retrieves the current channel topic.
std::string Channel::get_topic() const
{
	return _channel_topic;
}

// Returns true if a user limit is set.
bool Channel::has_user_limit() const
{

	return (_user_limit > 0);
}

// Checks if the user limit has been reached.
bool Channel::is_at_user_limit() const
{
	return (_connected_user_number == _user_limit);
}

// Sets the maximum number of users allowed in the channel.
void Channel::set_user_limit(const int new_limit) 
{
	_user_limit = new_limit;
}

// Gets the current user limit.
int Channel::get_user_limit() const
{
	return _user_limit;
}

// Enables invite-only mode for the channel.
void Channel::set_invite_only()
{
	_invite_only = true;
}

// Returns true if the channel is invite-only.
bool Channel::is_invite_only() const
{
	return _invite_only;
}

// Checks if a user is on the invitation list.
bool Channel::is_invited(const std::string& user_nick) const
{
	return (_channel_invitation_list.count(user_nick) > 0);
}

// Adds a user to the invitation list.
void Channel::add_invite(const std::string &user_nick)
{
	_channel_invitation_list.insert(user_nick);
}

// Returns true if a channel password is set.
bool Channel::has_password() const
{
	return !_channel_key.empty();
}

// Sets the channel password.
void Channel::set_password(const std::string &password)
{
	_channel_key = password;
}

// Validates a given password against the channel password.
// Returns true for any password if no channel password is set.
bool Channel::validate_password(const std::string &password) const
{
	return (!has_password() || (password == _channel_key));
}

// Gets the channel name.
const std::string& Channel::get_name() const
{
	return _channel_name;
}

// Returns a const reference to the list of channel members.
const std::set<std::string>& Channel::get_members() const
{
	return _channel_members_by_nickname;
}
