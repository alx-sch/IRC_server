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
	void delete_client(std::string nickname);
	void delete_client(int fd);
};

Server::Server(/* args */)
{
}

Server::~Server()
{
}


void Server::delete_client(std::string nickname)
{
	Client * client = nickname_client_map[nickname];
	nickname_client_map.erase(nickname);
	fd_client_map.erase(client->get_fd());
	delete client;
}

void Server::delete_client(int fd)
{
	Client * client = fd_client_map[fd];
	nickname_client_map.erase(client->get_nickname());
	fd_client_map.erase(fd);
	delete client;
}

# endif
