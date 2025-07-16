#ifndef SIGNALS_HPP
# define SIGNALS_HPP

# include <csignal> // for sig_atomic_t

// Global variable to control server running state (used in signal handler)
extern volatile sig_atomic_t	g_running;

void	handleSignal(int signum);

#endif
