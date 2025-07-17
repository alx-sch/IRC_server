#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>

class	User; // no include needed as only pointer is used

class Server
{
	private:
		// Disable default constructor and copying (makes no sense for a server)
		Server();
		Server(const Server& other); 
		Server&	operator=(const Server& other);

		int						_port;		// Server port
		int						_fd;		// Listening socket fd
		std::string				_password;	// Server password
		std::map<int, User*>	_users;		// Keep track of active clients <fd, User*>

		void					initSocket();

	public:
		Server(int port, const std::string& password);
		~Server();

		void					start();
		User*					getUser(int fd) const;
};

# endif
