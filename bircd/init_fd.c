
#include <string.h>
# include <sys/select.h>
#include "bircd.h"

/**
 init_fd - Initialize file descriptor sets for select() monitoring

 Prepares the fd_read and fd_write sets by scanning all file descriptors
 and adding active ones to the appropriate monitoring sets. Also updates
 the maximum file descriptor number for efficient select() operation.
 Called before each select() call to set up I/O monitoring.

 @param e 	Server environment containing fd array and sets
*/
void	init_fd(t_env *e)
{
	int	i;

	i = 0;
	e->max = 0;				// Reset maximum fd tracker
	FD_ZERO(&e->fd_read);	// Clear read monitoring set
	FD_ZERO(&e->fd_write);	// Clear write monitoring set
	
	// Scan all possible file descriptor slots
	while (i < e->maxfd)
	{
		// Only process active file descriptors (not free slots)
		if (e->fds[i].type != FD_FREE)
		{
			FD_SET(i, &e->fd_read);	// Monitor this fd for read readiness

			// Check if fd has data waiting to be written (never true in bircd)
			if (strlen(e->fds[i].buf_write) > 0)
			{
				FD_SET(i, &e->fd_write);	// Monitor this fd for write readiness
			}
			
			// Update highest active file descriptor number
			e->max = MAX(e->max, i);
		}
		i++;
	}
}
