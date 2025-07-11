#ifndef BIRCD_H
# define BIRCD_H

# include <sys/select.h>

# define FD_FREE	0
# define FD_SERV	1
# define FD_CLIENT	2

# define BUF_SIZE	4096

# define Xv(err,res,str)	(x_void(err,res,str,__FILE__,__LINE__))
# define X(err,res,str)		(x_int(err,res,str,__FILE__,__LINE__))
# define MAX(a,b)	((a > b) ? a : b)

# define USAGE		"Usage: %s port\n"

/**
 t_fd - File Descriptor Abstraction Structure

 Represents a polymorphic file descriptor with associated behavior
 and buffering capabilities for network I/O operations.

 This structure enables the server to handle different types of
 file descriptors (server socket, client connections) uniformly
 while providing type-specific read/write operations through
 function pointers.
*/
typedef struct s_fd
{
	int		type;					// File descriptor type (FD_FREE/FD_SERV/FD_CLIENT
	void	(*fct_read)();			// Function pointer for read operations
	void	(*fct_write)();			// Function pointer for write operations
	char	buf_read[BUF_SIZE + 1];	// Input buffer with null-terminator space
	char	buf_write[BUF_SIZE + 1];// Output buffer with null-terminator space
}		t_fd;

/**
 t_env - Server Environment Structure

 Central state management structure for the IRC server that maintains
 all connection information and select() monitoring data, enabling
 handling of multiple simultaneous client connections.
*/
typedef struct s_env
{
	t_fd	*fds;		// Array of file descriptor structures
	int		port;		// Server listening port number
	int		maxfd;		// Highest file descriptor number in use
	int		max;		// Maximum number of file descriptors
	int		r;			// Return value from select() system call
	fd_set	fd_read;	// File descriptor set for read monitoring
	fd_set	fd_write;	// File descriptor set for write monitoring
}		t_env;

void	init_env(t_env *e);
void	get_opt(t_env *e, int ac, char **av);
void	main_loop(t_env *e);
void	srv_create(t_env *e, int port);
void	srv_accept(t_env *e, int s);
void	client_read(t_env *e, int cs);
void	client_write(t_env *e, int cs);
void	clean_fd(t_fd *fd);
int		x_int(int err, int res, char *str, char *file, int line);
void	*x_void(void *err, void *res, char *str, char *file, int line);
void	init_fd(t_env *e);
void	do_select(t_env *e);
void	check_fd(t_env *e);

#endif
