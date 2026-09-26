#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "signals.h"
#include "parser.h"
#include "execute.h"
#include "jobs.h"
#include "history.h"
#include "alias.h"
#include "safety.h"
#include "linereader.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

void lsh_loop(void)
{
    char *line;
    ShellToken **tokens;

    while (!g_should_exit)
    {
        jobs_reap();
        lsh_print_prompt();
        line = lsh_read_interactive_line();
        if (line == NULL)
        {
            if (isatty(STDIN_FILENO))
            {
                printf("\n");
            }
            break;
        }

        history_add(line);
        tokens = lsh_split_line(line);
        lsh_execute_line(tokens);

        free(line);
        lsh_free_tokens(tokens);
    }
}

static int run_script_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        perror("apex-shell");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while (!g_should_exit && (read = getline(&line, &len, f)) != -1)
    {
        jobs_reap();
        line[strcspn(line, "\r\n")] = '\0';

        char *p = line;
        while (*p && isspace((unsigned char)*p))
        {
            p++;
        }

        if (*p == '\0' || *p == '#')
        {
            continue;
        }

        ShellToken **tokens = lsh_split_line(p);
        lsh_execute_line(tokens);
        lsh_free_tokens(tokens);
    }

    free(line);
    fclose(f);
    return g_last_exit_status;
}

static int run_command_string(const char *cmd)
{
    ShellToken **tokens = lsh_split_line((char *)cmd);
    lsh_execute_line(tokens);
    lsh_free_tokens(tokens);
    return g_last_exit_status;
}

int main(int argc, char **argv)
{
    jobs_init();
    lsh_init_signals();
    alias_init();
    safety_init();

    if (argc >= 3 && strcmp(argv[1], "-c") == 0)
    {
        run_command_string(argv[2]);
    }
    else if (argc >= 2 && argv[1][0] != '-')
    {
        run_script_file(argv[1]);
    }
    else
    {
        history_init();

        /* Auto-load ~/.apexrc on interactive startup if present */
        const char *home = getenv("HOME");
        if (home != NULL)
        {
            char rc_path[1024];
            snprintf(rc_path, sizeof(rc_path), "%s/.apexrc", home);
            if (access(rc_path, R_OK) == 0)
            {
                char *rc_args[] = {"source", rc_path, NULL};
                lsh_source(rc_args);
            }
        }

        lsh_loop();
        history_save();
        history_cleanup();
    }

    alias_cleanup();
    jobs_cleanup();
    return g_last_exit_status;
}
