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

		// IRC commands the server can handle
		enum	Cmd
		{
			UNKNOWN,
			NICK,		// Set user nickname
			USER,		// Set user username, hostname, servername, and realname
			PASS,		// Try to authenticate with server password
			QUIT,		// Disconnect from server
			PRIVMSG,	// Message to a user or channel
			NOTICE,		// Message to a user or channel, but triggers no auto-reply
			JOIN,		// Join a channel
			PART,		// Leave a channel
			TOPIC,
			KICK,
			INVITE,
			MODE
		};

		// === CommandRegistration.cpp ===

		static bool		handleNick(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleUser(User* user, const std::vector<std::string>& tokens);
		static bool		handlePass(Server* server, User* user, const std::vector<std::string>& tokens);

		// OTHER CPP FILES

		static bool		handleQuit(Server* server, User* user, const std::vector<std::string>& tokens);

		static bool		handleJoin(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleSingleJoin(Server* server, User* user, const std::string& channelName, const std::string& key);
		
		static bool		handlePart(Server* server, User* user, const std::vector<std::string>& tokens);
		
		static bool		handlePrivmsg(Server *server, User *user, const std::vector<std::string> &tokens);
		static bool		handleNotice(Server* server, User* user, const std::vector<std::string>& tokens);

		static bool		handleInvite(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleTopic(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleKick(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleMode(Server* server, User* user, const std::vector<std::string>& tokens);

		static std::vector<std::string>	tokenize(const std::string& message);
		static Cmd		getCmd(const std::string& message);
		static bool		checkRegistered(User* user, const std::string& command = "a command");
		static void		broadcastToChannel(Server* server, Channel* channel,
							const std::string& message, const std::string& excludeNick = "");
};

# endif
