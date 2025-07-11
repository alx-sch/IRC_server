
#include "bircd.h"

/**
 main_loop - Core event loop for the IRC server

 The loop continuously monitors all active file descriptors and
 processes I/O events as they occur.
 This is the heart of the event-driven architecture.
 
 @param e 	Server environment containing all connection state
*/
void	main_loop(t_env *e)
{
	while (1)
	{
		init_fd(e);		// Prepare fd sets for monitoring (add active fds)
		do_select(e);	// Block until at least one fd is ready for I/O
		check_fd(e);	// Process all ready file descriptors
	}
}
