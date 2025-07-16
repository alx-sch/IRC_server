#include <iostream>
#include "../include/signal.hpp"

volatile sig_atomic_t	g_running = 1;

// SIGINT signal handler to gracefully shut down the server
static void	handleSignal(int signum)
{
	(void)signum;

	std::cout << "\nShutting down server...\n";
	g_running = 0;
}

// Sets up SIGINT signal handler to gracefully shut down the server
void	setupSignalHandler()
{
	if (std::signal(SIGINT, handleSignal) == SIG_ERR)
		throw std::runtime_error("Failed to register signal handler (SIGINT)");
}
