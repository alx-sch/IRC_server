
#include "bircd.h"

/**
 check_fd - Process file descriptors ready for I/O operations

 Iterates through all active file descriptors and calls appropriate
 read/write handlers based on select() results. Uses polymorphic
 function pointers to handle different descriptor types uniformly.

 @param e 	Server environment containing fd sets and handler functions
*/
void	check_fd(t_env *e)
{
	int	i;

	i = 0;
	// Loop through all file descriptors up to maxfd
	// Stop early if all ready descriptors have been processed (r == 0)
	while ((i < e->maxfd) && (e->r > 0))
	{
		// Check if this fd is ready for reading
		if (FD_ISSET(i, &e->fd_read)) // FD_ISSET checks if the fd is part of the set. It's part of the select() system call.
			e->fds[i].fct_read(e, i);	// Call polymorphic read handler
	
		// Check if this fd is ready for writing
		if (FD_ISSET(i, &e->fd_write))
			e->fds[i].fct_write(e, i);	// Call polymorphic write handler

		// Decrement counter if this fd hac any activity
		// This optimization avoids unnecessary iterations
		if (FD_ISSET(i, &e->fd_read) || FD_ISSET(i, &e->fd_write))
			e->r--;	// Reduce remaining ready fc count
		i++; // Move to next fd
	}
}
