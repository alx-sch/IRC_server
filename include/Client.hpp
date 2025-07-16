#include <string>

class Client
{
private:
	int fd;
	std::string nickname;
	std::string username;
public:
	Client(/* args */);
	~Client();
	int get_fd();
};

Client::Client(/* args */)
{
}

Client::~Client()
{
}

int Client::get_fd()
{
	return fd;
}

