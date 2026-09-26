#ifndef CSHELL_SIGNALS_H
#define CSHELL_SIGNALS_H

#include <signal.h>

/* Global signal interruption flag */
extern volatile sig_atomic_t g_interrupted;

/* Initialize signal handlers for the interactive shell */
void lsh_init_signals(void);

#endif /* CSHELL_SIGNALS_H */
