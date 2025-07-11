
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bircd.h"

/**
 srv_accept - Accept new client connections on server socket

 Handles incoming connection requests by accepting the connection,
 logging client information, and setting up the file descriptor
 structure for the new client. Configures polymorphic read/write
 handlers to enable uniform processing in the main event loop.

 @param e 	Server environment containing fd array and connection state
 @param s 	Server socket file descriptor that has a pending connection
*/
void	srv_accept(t_env *e, int s)
{
	int					cs;			// Client socket file descriptor
	struct sockaddr_in	csin;		// Client address information
	socklen_t			csin_len;	// Size of client address structure

	// Prepare for accept() call
	csin_len = sizeof(csin);

	// Accept incoming connection and get client socket fd
	cs = X(-1, accept(s, (struct sockaddr*)&csin, &csin_len), "accept");

	// Log new client connection with IP and port information
	printf("New client #%d from %s:%d\n", cs,
		inet_ntoa(csin.sin_addr),	// Convert IP to string
		ntohs(csin.sin_port));		// Convert port to host byte order
	
	// Initialize the client's file descriptor structure
	clean_fd(&e->fds[cs]);				// Reset to clean state
	e->fds[cs].type = FD_CLIENT;		// Mark as client connection
	e->fds[cs].fct_read = client_read;	// Assign read handler
	e->fds[cs].fct_write = client_write;// Assign write handler
}
