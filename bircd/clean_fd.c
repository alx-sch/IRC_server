
#include <stdlib.h>
#include "bircd.h"

/**
 clean_fd - Reset file descriptor structure to unused state

 Clears a file descriptor entry by resetting its type to FD_FREE
 and nullifying function pointers. This prepares the structure
 for reuse when a connection is closed or needs to be reset.

 @param fd 	Pointer to the file descriptor structure to clean
*/
void	clean_fd(t_fd *fd)
{
	fd->type = FD_FREE;
	fd->fct_read = NULL;
	fd->fct_write = NULL;
}
