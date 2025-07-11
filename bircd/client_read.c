
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include "bircd.h"

/**
 client_read - Handle incoming data from a client connection

 Reads data from a client socket and broadcasts it to all other
 connected clients. If the client disconnects or an error occurs,
 cleans up the connection and notifies about the disconnection.

 @param e 	Server environment containing all connection information
 @param cs 	Client socket file descriptor to read from
*/
void	client_read(t_env *e, int cs)
{
	int	r;
	int	i;

	// Attempt to receive data from the client
	r = recv(cs, e->fds[cs].buf_read, BUF_SIZE, 0);

	// Check for disconnection or error
	if (r <= 0)
	{
		close(cs); // Close the socket
		clean_fd(&e->fds[cs]); // Reset fd structure
		printf("client #%d gone away\n", cs); // Log disconnection
	}
	else
	{
		// Broadcast message to all other clients
		// NOTE: Writing is done directly here, bypassing buf_write system
		i = 0;
		while (i < e->maxfd)
		{
			// Send to client connections, but not back to sender
			if ((e->fds[i].type == FD_CLIENT) && (i != cs))
				send(i, e->fds[cs].buf_read, r, 0); // Direct send - no buffering
			i++;  // Check next fd
		}
	}
}
