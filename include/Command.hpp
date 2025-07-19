#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>
# include <vector>

class	Command
{
	public:
		enum	Type
		{
			UNKNOWN,
			NICK,
			USER,
			PASS,
			PING,
			JOIN,
			PART,
			QUIT,
			PRIVMSG,
			TOPIC,
			KICK,
			INVITE,
			MODE
		};

		static std::vector<std::string>	tokenize(const std::string& message);
		static Type						getType(const std::string& message);

	private:
		// Pure utility class, no need for instantiation
		Command();
		Command(const Command& other);
		Command&	operator=(const Command& other);
		~Command();
};

#endif
