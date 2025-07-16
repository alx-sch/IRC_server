#include <iostream>
#include "../include/signal.hpp"

volatile sig_atomic_t	g_running = 1;

void handleSignal(int signum)
{
	(void)signum;

	std::cout << "\nShutting down server...\n";
	g_running = 0;
}
