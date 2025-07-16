class Command
{
private:
	enum class Type {
		JOIN,
		USER,
		KICK,
		INVITE,
		TOPIC,
		MODE_CHANNEL,
		MODE_NICK,
		LIST
	};
	
public:
	Command(/* args */);
	~Command();
};

Command::Command(/* args */)
{
}

Command::~Command()
{
}
