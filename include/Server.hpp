#include <map>
#include "Client.hpp"

#ifndef SERVER_HPP
# define SERVER_HPP

class Server
{
private:
	std::map<int, Client*> fd_client_map;
	std::map<std::string, Client*> nickname_client_map;
public:
	Server(/* args */);
	~Server();
};

Server::Server(/* args */)
{
}

Server::~Server()
{
}


# endif
