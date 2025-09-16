#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>
# include <vector>
# include <stdlib.h>

class	Server;
class	User;
class	Channel;

class	Command
{
	public:
		static bool		handleCommand(Server* server, User* user, std::vector<std::string>& tokens);
		static void		broadcastToChannel(Server* server, Channel* channel,
									const std::string& message, const std::string& excludeNick = "");
		static std::vector<std::string>	tokenize(const std::string& message);

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
			TOPIC,		// Set or view channel topic
			KICK,		// Kick a user from a channel (ops only)
			INVITE,		// Invite a user to a channel
			MODE		// Change channel or user mode
		};

		// === CommandRegistration.cpp ===

		static void		handleNick(Server* server, User* user, const std::vector<std::string>& tokens);
		static void		handleUser(User* user, const std::vector<std::string>& tokens);
		static void		handlePass(Server* server, User* user, const std::vector<std::string>& tokens);

		// === CommandChannel.cpp ===

		static bool		handleJoin(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleSingleJoin(Server* server, User* user, const std::string& channelName, const std::string& key);
		static bool		handleSinglePart(Server* server, User* user, const std::string& channelName, const std::string& partMessage);
		static bool		handlePart(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleInvite(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleTopic(Server* server, User* user, const std::vector<std::string>& tokens);
		static bool		handleKick(Server* server, User* user, const std::vector<std::string>& tokens);

		// === CommandModes.cpp ===
		
		static bool		handleMode(Server* server, User* user, const std::vector<std::string>& tokens);
		static Channel*	validateChannelAndUser(Server* server, User* user, const std::string& target);
		static void		formatChannelModes(Channel* channel, User* user, std::string& modes, std::string& params,
											std::string& paramsLogging);
		static void		sendModeReply(User* user, const std::string& target, const std::string& modes,
										const std::string& params, std::string& paramsLogging);
		
		static bool		handleModeChanges(Server* server, User* user, Channel* channel, const std::vector<std::string>& tokens);
		static bool		applyChannelMode(Server* server, User* user, Channel* channel, char mode, bool adding,
										const std::vector<std::string>& tokens, size_t& paramIndex, std::string& modeParams);
		static bool		applySimpleMode(Channel* channel, User* user, char mode, bool adding);
		static bool		applyUserLimit(Channel* channel, User* user, bool adding, const std::vector<std::string>& tokens,
										size_t& paramIndex, std::string& modeParams);
		static bool		applyChannelKey(Channel* channel, User* user, bool adding, const std::vector<std::string>& tokens,
										size_t& paramIndex, std::string& modeParams);
		static bool		applyOperator(Server* server, Channel* channel, User* user, bool adding,
										const std::vector<std::string>& tokens, size_t& paramIndex, std::string& modeParams);

		// === CommandMessaging.cpp ===
		
		static void		handlePrivmsg(Server *server, User *user, const std::vector<std::string> &tokens);
		static void		handleNotice(Server* server, User* user, const std::vector<std::string>& tokens);
		static void		handleMessage(Server* server, User* user, const std::vector<std::string>& tokens,
							 			const std::string& commandName);

		// === CommandConnection.cpp ===

		static void		handleQuit(Server* server, User* user, const std::vector<std::string>& tokens);

		// === CommandUtils.cpp ===

		static Cmd		getCmd(const std::vector<std::string>& tokens);
		static bool		checkRegistered(User* user, const std::string& command = "a command");
		static std::vector<std::string>	splitCommaList(const std::string& list);
};

# endif
