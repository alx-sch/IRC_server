#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <set>
# include <string>

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
			JOIN_BAD_KEY
		};

		const std::string&	get_name() const;
		Channel*			get_channel(const std::string& channelName) const;
		const std::set<std::string>&	get_members() const;
		std::string			get_names_list() const;
		std::string			get_mode_string(const User* user) const;
		
	
		void		add_user(const std::string& user_nick);
		void		remove_user(const std::string& user_nick);
		bool		is_user_member(const std::string& user_nick) const;

		void		make_user_operator(const std::string& user_nick);
		void		remove_user_operator_status(const std::string& user_nick);
		bool		is_user_operator(const std::string& user_nick) const;

		bool		can_user_join(const std::string& user_nick,
						const std::string& provided_key, JoinResult& result) const;

		void		set_topic(const std::string& topic, const std::string& set_by);
		const std::string&	get_topic() const;
		std::string	get_topic_set_info() const;
		void		set_topic_protection(bool enable = true);
		bool		has_topic_protection() const;

		bool		has_user_limit() const;
		bool		is_at_user_limit() const;
		void		set_user_limit(const int new_limit);
		int			get_user_limit() const;

		void		set_invite_only(bool enable = true);
		bool		is_invite_only() const;
		bool		is_invited(const std::string& user_nick) const;
		void		add_invite(const std::string& user_nick);

		bool		has_password() const;
		void		set_password(const std::string& password);
		const std::string&	get_password() const;
		bool		validate_password(const std::string& password) const;

	private:
		Channel();
		Channel(const Channel& other);
		Channel&	operator=(const Channel& other);

		std::string				_channel_name;
		std::string				_channel_topic;
		std::string				_channel_topic_set_by;
		int						_channel_topic_set_at;
		std::set<std::string>	_channel_members_by_nickname;
		std::set<std::string>	_channel_operators_by_nickname;
		std::set<std::string>	_channel_invitation_list;

		int						_connected_user_number;
		// MODE RELATED VARS
		int						_user_limit;	// set by l
		bool					_invite_only;	// set by i
		bool					_topic_protection;	// set by t
		std::string				_channel_key;	// password set by k
};

#endif
