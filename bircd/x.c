
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bircd.h"


/**
 x_int - Error checking wrapper for functions returning integers

 Checks if a function call returned an error value and handles
 error reporting with file/line information. Used by the `X()` macro
 to provide consistent error handling throughout the codebase.

 @param err 	Expected error value (usually -1)
 @param res 	Result returned by the system call
 @param str 	Descriptive name of the function that was called
 @param file 	Source file where the error occurred (__FILE__)
 @param line 	Line number where the error occurred (__LINE__)

 @returns		The original result if no error, exits program if error
*/
int	x_int(int err, int res, char *str, char *file, int line)
{
	// Check if the result matches the expected error value
	if (res == err)
	{
		// Print detailed error message with location and system error
		fprintf(stderr, "%s error (%s, %d): %s\n",
			str,	// Function name (e.g., "socket")
			file,	// Source file (e.g., "srv_create.c")  
			line,	// Line number in the source file
			strerror(errno));	// System error description
		exit (1);
	}
	return (res);
}

/**
 x_void - Error checking wrapper for functions returning pointers

 Similar to `x_int` but handles functions that return pointers.
 Used by the `Xv()` macro for functions like `malloc()` that return
 `NULL` on error instead of `-1`.

 @param err 	Expected error value (usually NULL)
 @param res 	Result returned by the function call
 @param str 	Descriptive name of the function that was called
 @param file 	Source file where the error occurred (__FILE__)
 @param line 	Line number where the error occurred (__LINE__)

 @returns		The original result if no error, exits program if error
*/
void	*x_void(void *err, void *res, char *str, char *file, int line)
{
	if (res == err)
	{
		fprintf(stderr, "%s error (%s, %d): %s\n",
			str,
			file,
			line,
			strerror(errno));
		exit (1);
	}
	return (res);
}
