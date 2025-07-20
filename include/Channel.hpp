#include <set>
#include <string>

#ifndef CHANNEL_HPP
#define CHANNEL_HPP
class Channel
{
private:
	std::string _channel_name;
	std::string _channel_topic;
	std::set<std::string> _channel_members_by_nickname;
	std::set<std::string> _channel_operators_by_nickname;
	std::set<std::string> _channel_invitation_list;

	int _connected_user_number;
	// MODE RELATED VARS
	int _user_limit; // set by l
	bool _invite_only; // set by i
	bool _topic_protection; // set by t
	std::string _channel_key; // password set by k

public:
	Channel(std::string name);
	~Channel();

    // Getters
    const std::string& get_name() const;

    Channel* get_channel(const std::string& channelName) const;

	void add_user(const std::string& user_nick);
	void remove_user(const std::string& user_nick);
	bool is_user_member(const std::string& user_nick);

	void make_user_operator(const std::string& user_nick);
	void remove_user_operator_status(const std::string& user_nick);
	bool is_user_operator(const std::string& user_nick);


	bool can_user_join(const std::string& user_nick, const std::string& provided_key) const;

	void set_topic(const std::string& topic);
	bool has_topic_protection() const;
	std::string get_topic() const;
	
	bool has_user_limit() const;
	bool is_at_user_limit() const;
	void set_user_limit(const int new_limit);
	int get_user_limit() const;

	void set_invite_only();
	bool is_invite_only() const;
	bool is_invited(const std::string& nickname) const;


	bool has_password() const;
	void set_password(const std::string& password);
	bool validate_password(const std::string& password) const;

};

#endif
