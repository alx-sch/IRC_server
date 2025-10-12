#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <set>
# include <map>
# include <string>
# include <ctime>	// time_t

class	User;

class	Channel
{
	public:
		Channel(std::string name);
		~Channel();

		// Reasons a user might not be able to join a channel
		enum	JoinResult
		{
			JOIN_INVITE_ONLY,
			JOIN_FULL,
			JOIN_BAD_KEY,
			JOIN_MAX_CHANNELS
		};

		const std::string&				get_name() const;
		const std::string&				get_name_lower() const;
		const std::map<std::string, User*>&	get_members() const;
		std::string						get_names_list() const;
		std::string						get_mode_string(const User* user) const;
		int								get_connected_user_number() const;

		void	add_user(User *user);
		void	remove_user(User *user);
		bool	is_user_member(User *user) const;

		void	make_user_operator(User *user);
		void	remove_user_operator_status(User *user);
		bool	is_user_operator(const User *user) const;

		bool	can_user_join(User* user, const std::string& provided_key,
							JoinResult& result) const;

		void	set_topic(const std::string& topic, const std::string& set_by);
		const std::string&	get_topic() const;
		std::string			get_topic_set_info() const;
		void	set_topic_protection(bool enable = true);
		bool	has_topic_protection() const;

		bool	has_user_limit() const;
		bool	is_at_user_limit() const;
		void	set_user_limit(const int new_limit);
		int		get_user_limit() const;

		void	set_invite_only(bool enable = true);
		bool	is_invite_only() const;
		bool	is_invited(const std::string& user_nick) const;
		void	add_invite(const std::string& user_nick);

		bool	has_password() const;
		void	set_password(const std::string& password);
		const std::string&	get_password() const;
		bool	validate_password(const std::string& password) const;

	private:
		Channel();
		Channel(const Channel& other);
		Channel&	operator=(const Channel& other);

		std::string				_channel_name;
		std::string				_channel_name_lower;
		std::string				_channel_topic;
		std::string				_channel_topic_set_by;
		time_t					_channel_topic_set_at;
		std::map<std::string, User*>	_channel_members_by_nickname;
		std::map<std::string, User*>	_channel_operators_by_nickname;
		std::set<std::string>	_channel_invitation_list;

		// MODE RELATED VARS
		int						_user_limit;	// set by l
		bool					_invite_only;	// set by i
		bool					_topic_protection;	// set by t
		std::string				_channel_key;	// password set by k
};

#endif
