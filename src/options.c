#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

ShellOptions g_shell_opts = {
    .errexit = 0,
    .xtrace = 0,
    .nounset = 0,
    .pipefail = 0
};

static void set_option_by_name(const char *name, int enable)
{
    if (strcmp(name, "errexit") == 0) g_shell_opts.errexit = enable;
    else if (strcmp(name, "xtrace") == 0) g_shell_opts.xtrace = enable;
    else if (strcmp(name, "nounset") == 0) g_shell_opts.nounset = enable;
    else if (strcmp(name, "pipefail") == 0) g_shell_opts.pipefail = enable;
    else fprintf(stderr, "set: unknown option '%s'\n", name);
}

static int set_option_by_char(char c, int enable)
{
    switch (c)
    {
        case 'e':
            g_shell_opts.errexit = enable;
            return 0;
        case 'x':
            g_shell_opts.xtrace = enable;
            return 0;
        case 'u':
            g_shell_opts.nounset = enable;
            return 0;
        default:
            fprintf(stderr, "set: invalid option '%c%c'\n", enable ? '-' : '+', c);
            return 1;
    }
}

int lsh_set(char **args)
{
    if (args[1] == NULL)
    {
        printf("Current Shell Options:\n");
        printf("  errexit  (-e) : %s\n", g_shell_opts.errexit ? "on" : "off");
        printf("  xtrace   (-x) : %s\n", g_shell_opts.xtrace ? "on" : "off");
        printf("  nounset  (-u) : %s\n", g_shell_opts.nounset ? "on" : "off");
        printf("  pipefail      : %s\n", g_shell_opts.pipefail ? "on" : "off");
        printf("\nEnvironment Variables:\n");
        if (environ)
        {
            for (char **env = environ; *env != NULL; env++)
            {
                printf("%s\n", *env);
            }
        }
        return 0;
    }

    for (int i = 1; args[i] != NULL; i++)
    {
        const char *arg = args[i];
        if (arg[0] == '-' || arg[0] == '+')
        {
            int enable = (arg[0] == '-');
            if (strcmp(arg, "-o") == 0 || strcmp(arg, "+o") == 0)
            {
                if (args[i + 1] != NULL)
                {
                    i++;
                    set_option_by_name(args[i], enable);
                }
                else
                {
                    fprintf(stderr, "set: %s requires an argument\n", arg);
                    return 1;
                }
                continue;
            }

            for (int j = 1; arg[j] != '\0'; j++)
            {
                if (set_option_by_char(arg[j], enable) != 0)
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

