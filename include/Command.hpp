#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>
# include <vector>

class	Server;
class	User;
class	Channel;

class	Command
{
	public:
		static bool	handleCommand(Server* server, User* user, const std::string& message);

	private:
		// Pure utility class, no need for instantiation
		Command();
		Command(const Command& other);
		Command&	operator=(const Command& other);
		~Command();

		enum	Type
		{
			UNKNOWN,
			NICK,		// Set user nickname
			USER,		// Set user username and realname
			PASS,		// Try to authenticate with server password
			PRIVMSG,	// Message to a user or channel
			NOTICE,		// Notice to a user or channel
			JOIN,		// Join a channel
			PART,		// Leave a channel
			TOPIC,
			KICK,
			INVITE,
			MODE
		};

		static bool		handleNick(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleUser(User* user, const std::vector<std::string>& tokens);
		static bool		handlePass(Server* server, User* user, const std::vector<std::string>& tokens);
		
		static bool		handleJoin(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handlePart(Server* server, User* user, const std::vector<std::string>& tokens);
		
		static void		broadcastToChannel(Server* server, Channel* channel,
							const std::string& message, const std::string& excludeNick = "");
		static bool		handlePrivmsg(Server *server, User *user, const std::vector<std::string> &tokens);
		static bool		handleNotice(Server* server, User* user, const std::vector<std::string>& tokens);

		static bool		handleInvite(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleTopic(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleKick(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleMode(Server* server, User* user, const std::vector<std::string>& tokens);

		static std::vector<std::string>	tokenize(const std::string& message);
		static Type		getType(const std::string& message);
};

#endif
