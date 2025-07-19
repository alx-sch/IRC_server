#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>
# include <vector>

class	Server;
class	User;

class	Command
{
	public:
		static bool	handleCommand(Server* server, User* user, int fd, const std::string& message);

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
			PASS,
			PING,
			JOIN,
			PART,
			QUIT,
			PRIVMSG,	// Message to a user or channel
			TOPIC,
			KICK,
			INVITE,
			MODE
		};

		static bool						handleNick(User* user, const std::vector<std::string>& tokens);
		static bool						handleUser(User* user, const std::vector<std::string>& tokens);
		
		static std::vector<std::string>	tokenize(const std::string& message);
		static Type						getType(const std::string& message);
	
};

#endif
