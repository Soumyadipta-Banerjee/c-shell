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
    .nounset = 0
};

int lsh_set(char **args)
{
    if (args[1] == NULL)
    {
        printf("Current Shell Options:\n");
        printf("  errexit (-e) : %s\n", g_shell_opts.errexit ? "on" : "off");
        printf("  xtrace  (-x) : %s\n", g_shell_opts.xtrace ? "on" : "off");
        printf("  nounset (-u) : %s\n", g_shell_opts.nounset ? "on" : "off");
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
        if (arg[0] == '-')
        {
            if (strcmp(arg, "-o") == 0 && args[i + 1] != NULL)
            {
                i++;
                if (strcmp(args[i], "errexit") == 0) g_shell_opts.errexit = 1;
                else if (strcmp(args[i], "xtrace") == 0) g_shell_opts.xtrace = 1;
                else if (strcmp(args[i], "nounset") == 0) g_shell_opts.nounset = 1;
                else fprintf(stderr, "set: unknown option '%s'\n", args[i]);
                continue;
            }

            for (int j = 1; arg[j] != '\0'; j++)
            {
                switch (arg[j])
                {
                    case 'e':
                        g_shell_opts.errexit = 1;
                        break;
                    case 'x':
                        g_shell_opts.xtrace = 1;
                        break;
                    case 'u':
                        g_shell_opts.nounset = 1;
                        break;
                    default:
                        fprintf(stderr, "set: invalid option '-%c'\n", arg[j]);
                        return 1;
                }
            }
        }
        else if (arg[0] == '+')
        {
            if (strcmp(arg, "+o") == 0 && args[i + 1] != NULL)
            {
                i++;
                if (strcmp(args[i], "errexit") == 0) g_shell_opts.errexit = 0;
                else if (strcmp(args[i], "xtrace") == 0) g_shell_opts.xtrace = 0;
                else if (strcmp(args[i], "nounset") == 0) g_shell_opts.nounset = 0;
                else fprintf(stderr, "set: unknown option '%s'\n", args[i]);
                continue;
            }

            for (int j = 1; arg[j] != '\0'; j++)
            {
                switch (arg[j])
                {
                    case 'e':
                        g_shell_opts.errexit = 0;
                        break;
                    case 'x':
                        g_shell_opts.xtrace = 0;
                        break;
                    case 'u':
                        g_shell_opts.nounset = 0;
                        break;
                    default:
                        fprintf(stderr, "set: invalid option '+%c'\n", arg[j]);
                        return 1;
                }
            }
        }
    }

    return 0;
}
