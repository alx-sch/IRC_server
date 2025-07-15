
#include <stdlib.h>
#include "bircd.h"

/**
 do_select - Perform I/O multiplexing using select() system call

 Monitors multiple file descriptors simultaneously for I/O readiness.
 Blocks until at least one file descriptor becomes ready for reading
 or writing operations. This is the core mechanism that allows the
 server to handle multiple client connections efficiently.

 @param e 	Server environment containing fd sets and max fd number
*/
void	do_select(t_env *e)
{
	// select()
	// 1. range of fds to check, excluding e->max + 1
	// 2. set of fds to check for reading
	// 3. set of fds to check for writing (not used in bircd; always empty due to no bufering)
	// 4. set of fds to check for exceptions (not used in bircd; always NULL)
	// 5. timeout (NULL means block indefinitely until at least one fd is ready)
	e->r = select(e->max + 1, &e->fd_read, &e->fd_write, NULL, NULL);
}
