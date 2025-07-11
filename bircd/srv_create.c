
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include "bircd.h"

/**
 srv_create - Create and configure the server socket

 Sets up a TCP server socket that listens for incoming client connections.
 Creates the socket, binds it to the specified port on all available
 interfaces, and configures it for listening. Also sets up the polymorphic
 file descriptor structure to handle incoming connections uniformly.

 @param e 		Server environment containing fd array and connection state
 @param port 	Port number to bind the server socket to
*/
void	srv_create(t_env *e, int port)
{
	int					s;		// Server socket file descriptor
	struct sockaddr_in	sin;	// Server address structure
	struct protoent		*pe;	// Protocol entry for TCP

	// Get TCP protocol information from system database
	pe = (struct protoent*)Xv(NULL, getprotobyname("tcp"), "getprotobyname");

	// Create TCP socket (IPv4, stream-based, TCP protocol)
	s = X(-1, socket(PF_INET, SOCK_STREAM, pe->p_proto), "socket");

	// Configure server address structure
	sin.sin_family = AF_INET;		// IPv4 address family
	sin.sin_addr.s_addr = INADDR_ANY;	// Listen on all available interfaces (0.0.0.0)
	sin.sin_port = htons(port);		// Convert port to network byte order

	// Bind socket to address and port
	X(-1, bind(s, (struct sockaddr*)&sin, sizeof(sin)), "bind");

	// Start listening for connections (queue up to 42 pending connections)
	X(-1, listen(s, 42), "listen");

	// Configure polymorphic file descriptor structure
	e->fds[s].type = FD_SERV;			// Mark as server socket
	e->fds[s].fct_read = srv_accept;	// Assign connection acceptance handler
	// Note: fct_write not set - server socket only accepts, never writes
}
