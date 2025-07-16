#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>

# include "User.hpp"

class Server
{
	private:
		int					_port;
		int					_fd;
		std::string			_password;
		std::vector<User*>	_users; // Keep track of active clients

		void				initSocket();

		// Not a great idea to copy a user -> Disable copy/assignment
		Server();
		Server(const Server& other); 
		Server&	operator=(const Server& other);

	public:
		Server(int port, const std::string& password);
		~Server();

		void			start();
};

# endif
