
#include <stdio.h>
#include <stdlib.h>
#include "bircd.h"

/**
 get_opt - Parse command line arguments and validate usage

 Validates that exactly one argument (port number) is provided and
 stores the port number in the server environment. Exits with error
 message if incorrect number of arguments is provided.

 @param e 	Server environment structure to store the port
 @param ac 	Argument count from main()
 @param av 	Argument vector from main()
*/
void	get_opt(t_env *e, int ac, char **av)
{
	if (ac != 2)
	{
		fprintf(stderr, USAGE, av[0]);
		exit(1);
	}
	// Convert port string to integer and store in environment
	e->port = atoi(av[1]);	  // No validation of port range
}

