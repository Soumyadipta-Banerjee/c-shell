#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "safety.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int safemode_enabled = 1;

void safety_init(void)
{
    safemode_enabled = 1;
}

int lsh_safemode(char **args)
{
    if (args[1] == NULL || strcmp(args[1], "status") == 0)
    {
        printf("Safety Shield: %s\n", safemode_enabled ? "ENABLED (protecting root, home, and recursive targets)" : "DISABLED");
        return 0;
    }

    if (strcmp(args[1], "on") == 0 || strcmp(args[1], "enable") == 0)
    {
        safemode_enabled = 1;
        printf("Safety Shield: ENABLED\n");
        return 0;
    }

    if (strcmp(args[1], "off") == 0 || strcmp(args[1], "disable") == 0)
    {
        safemode_enabled = 0;
        printf("Safety Shield: DISABLED\n");
        return 0;
    }

    fprintf(stderr, "Usage: safemode [on|off|status]\n");
    return 1;
}

int safety_check_command(char **args)
{
    if (!safemode_enabled || !args || !args[0])
    {
        return 0;
    }

    if (strcmp(args[0], "rm") != 0)
    {
        return 0;
    }

    int is_recursive = 0;
    int has_dangerous_target = 0;
    const char *dangerous_target = NULL;
    char *home = getenv("HOME");

    for (int i = 1; args[i] != NULL; i++)
    {
        if (args[i][0] == '-')
        {
            if (strchr(args[i], 'r') || strchr(args[i], 'R') || strcmp(args[i], "--recursive") == 0)
            {
                is_recursive = 1;
            }
        }
        else
        {
            const char *target = args[i];
            if (strcmp(target, "/") == 0 || strcmp(target, "/*") == 0 ||
                strcmp(target, ".") == 0 || strcmp(target, "..") == 0 ||
                strcmp(target, "./*") == 0 || strcmp(target, "~") == 0 ||
                strcmp(target, "~/*") == 0 || strcmp(target, "*") == 0 ||
                (home && strcmp(target, home) == 0))
            {
                has_dangerous_target = 1;
                dangerous_target = target;
            }
        }
    }

    if (is_recursive && has_dangerous_target)
    {
        if (isatty(STDIN_FILENO))
        {
            fprintf(stderr, "\n⚠️  [SAFETY SHIELD] Destructive command detected ('rm' recursive on '%s')!\n", dangerous_target);
            fprintf(stderr, "Are you sure you want to proceed? Type 'yes' to confirm: ");
            fflush(stderr);
            char conf[32];
            if (fgets(conf, sizeof(conf), stdin) && strcmp(conf, "yes\n") == 0)
            {
                return 0;
            }
            fprintf(stderr, "apex-shell: execution aborted by Safety Shield.\n");
            return 1;
        }
        else
        {
            fprintf(stderr, "apex-shell: [SAFETY SHIELD] Blocked destructive recursive deletion on '%s'.\n", dangerous_target);
            return 1;
        }
    }

    return 0;
}
