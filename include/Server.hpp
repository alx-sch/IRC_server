#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>

class	User;	//no include needed as only pointer is used

class Server
{
	private:
		// Disable default constructor and copying (makes no sense for a server)
		Server();
		Server(const Server& other); 
		Server&	operator=(const Server& other);

		int								_port;		// Server port
		int								_fd;		// Listening socket fd
		std::string						_password;	// Server password
		std::map<int, User*>			_usersFd;	// Keep track of active users by fd
		std::map<std::string, User*>	_usersNick;	// Keep track of active users by nickname

		void		initSocket();					// Initializes the server socket

	public:
		Server(int port, const std::string& password);
		~Server();

		void		start();	// Starts the server loop, accepting incoming connections and managing users

		void		delete_client(int fd);
		void		delete_client(std::string nickname);
	
		User*		getUser(int fd) const;
};

void Server::delete_client(std::string nickname)
{
	User * user = _usersNick[nickname];
	_usersNick.erase(nickname);
	_usersFd.erase(user->getFd());
	delete user;
}

void Server::delete_client(int fd)
{
	User * user = _usersFd[fd];
	_usersNick.erase(user->getNickname());
	_usersFd.erase(fd);
	delete user;
}

# endif
