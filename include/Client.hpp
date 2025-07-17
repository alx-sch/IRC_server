#include <string>
#include <vector>

#ifndef CLIENT_HPP
#define CLIENT_HPP
class Client
{
private:
	int fd; //returned during the connection
	std::string nickname; // required but must be unique
	std::string username; // required
	std::string realname; // required - usually unused
	std::string hostname; // basically ip address -- should we do full resolution?
	std::vector<std::string> op_channels; //channels where this user has operator privileges
public:
	Client(/* args */);
	~Client();
	int get_fd();
	std::string get_nickname();
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

std::string Client::get_nickname()
{
	return nickname;
}

#endif