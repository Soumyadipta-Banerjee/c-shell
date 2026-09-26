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
        write(STDOUT_FILENO, "\n", 1);
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
}
