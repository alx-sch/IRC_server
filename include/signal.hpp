#ifndef SIGNALS_HPP
# define SIGNALS_HPP

# include <csignal>	// for sig_atomic_t, SIGINT, SIG_ERR

// Global variable to control server running state (used in signal handler)
extern volatile sig_atomic_t	g_running;

void	setupSignalHandler();

#endif
