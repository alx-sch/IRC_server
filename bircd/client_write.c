
#include <sys/socket.h>
#include "bircd.h"

/**
 Not implemented. `bircd` brodcasts messages immediately in client_read.c
 (no private messages, no buffering, no server-generated messages).
 --> Simple "echo" server.
 --> does not handle partial data, low bandwith issues, etc.
*/
void	client_write(t_env *e, int cs)
{
}
