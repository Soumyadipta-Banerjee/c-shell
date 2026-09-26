#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "signals.h"
#include "parser.h"
#include "execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void lsh_loop(void)
{
    char *line;
    ShellToken **tokens;

    while (!g_should_exit)
    {
        lsh_print_prompt();
        line = lsh_read_line();
        if (line == NULL)
        {
            if (isatty(STDIN_FILENO))
            {
                printf("\n");
            }
            break;
        }
        tokens = lsh_split_line(line);
        lsh_execute_line(tokens);

        free(line);
        lsh_free_tokens(tokens);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    lsh_init_signals();
    lsh_loop();
    return g_last_exit_status;
}
