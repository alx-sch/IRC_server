
#include <stdlib.h>
#include <sys/resource.h>
#include "bircd.h"

/**
 init_env - Initialize server environment and file descriptor management

 Sets up the server's file descriptor array by querying the system's
 file descriptor limit, allocating memory for the fd array, and
 initializing all slots to unused state.

 @param e 	Server environment structure to initialize
*/
void	init_env(t_env *e)
{
	int				i;
	struct rlimit	rlp;

	// Store current (soft) limit as our maximum fd capacity
	X(-1, getrlimit(RLIMIT_NOFILE, &rlp), "getrlimit"); // Error handling for getrlimit failure

	// Store current (soft)
	e->maxfd = rlp.rlim_cur; // Typically 1024 or higher; it's higher in Codespace and would result in malloc failure --> capped to MAX_FD
	if (e->maxfd > MAX_FD)
		e->maxfd = MAX_FD; // Cap to defined maximum for our server

	// Allocate memory for file descriptor array (one t_fd per possible fd)
	e->fds = (t_fd*)Xv(NULL, malloc(sizeof(*e->fds) * e->maxfd), "malloc");

	// Initialize all file descriptor slots to unused state
	i = 0;
	while (i < e->maxfd)
	{
		clean_fd(&e->fds[i]);
		i++;
	}
}
