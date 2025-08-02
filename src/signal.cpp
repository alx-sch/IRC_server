#include <iostream>
#include "../include/signal.hpp"
#include "../include/utils.hpp"		// logServerMessage()

volatile sig_atomic_t	g_running = 1;

// Handles `SIGINT` (Ctrl+C)
static void	handleSignal(int signum)
{
	(void)signum;

	std::cout << std::endl;  // Just a newline for clean output after Ctrl+C
	logServerMessage("Shutting down server...");
	g_running = 0;
}

// Sets up a signal handler for `SIGINT` to gracefully shut down the server.
void	setupSignalHandler()
{
	if (std::signal(SIGINT, handleSignal) == SIG_ERR)
		throw std::runtime_error("Failed to register signal handler (SIGINT)");
}
