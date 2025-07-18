#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <sys/select.h>	// for fd_set

class	User;	// no include needed as only pointer is used

class Server
{
	public:
		Server(int port, const std::string& password);
		~Server();

		void	run();

	private:
		// Disable default constructor and copying (makes no sense for a server)
		Server();
		Server(const Server& other); 
		Server&	operator=(const Server& other);

		int								_port;		// Server port
		int								_fd;		// server socket fd (listening socket)
		std::string						_password;	// Server password
		std::map<int, User*>			_usersFd;	// Keep track of active users by fd
		std::map<std::string, User*>	_usersNick;	// Keep track of active users by nickname

		// === ServerSocket.cpp ===

		void	initSocket();
		int		prepareReadSet(fd_set& readFds);

		// === ServerUser.cpp ===

		void	acceptNewUser();
		void	handleReadyUsers(fd_set& readFds);
		bool	handleUserInput(int fd);
		void	broadcastMessage(int senderFd, const std::string& nick, const std::string& message);
		void	handleSendError(int fd, const std::string& nick);

		void	deleteUser(int fd);
		void	deleteUser(const std::string& nickname);
		User*	getUser(int fd) const;
		User*	getUser(const std::string& nickname) const;
};

# endif
