#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "signals.h"
#include <unistd.h>
#include <string.h>

volatile sig_atomic_t g_interrupted = 0;

static void sigint_handler(int signo)
{
    (void)signo;
    g_interrupted = 1;
    if (isatty(STDIN_FILENO))
    {
        ssize_t w = write(STDOUT_FILENO, "\n", 1);
        (void)w;
    }
}

void lsh_init_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    struct sigaction sa_tstp;
    memset(&sa_tstp, 0, sizeof(sa_tstp));
    sa_tstp.sa_handler = SIG_IGN;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    /* Ignore terminal I/O signals to safely hand off foreground control */
    struct sigaction sa_ign;
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = 0;
    sigaction(SIGTTIN, &sa_ign, NULL);
    sigaction(SIGTTOU, &sa_ign, NULL);
}
