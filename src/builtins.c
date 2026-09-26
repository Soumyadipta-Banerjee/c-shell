#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "builtins.h"
#include "execute.h"
#include "jobs.h"
#include "telemetry.h"
#include "history.h"
#include "alias.h"
#include "safety.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

extern char **environ;

char *builtin_str[] = {
    "cd",
    "pwd",
    "help",
    "exit",
    "jobs",
    "export",
    "unset",
    "env",
    "sysinfo",
    "history",
    "alias",
    "unalias",
    "safemode",
    "fg",
    "bg",
    "kill",
    "source",
    "."
};

int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_pwd,
    &lsh_help,
    &lsh_exit,
    &lsh_jobs,
    &lsh_export,
    &lsh_unset,
    &lsh_env,
    &lsh_sysinfo,
    &lsh_history,
    &lsh_alias,
    &lsh_unalias,
    &lsh_safemode,
    &lsh_fg,
    &lsh_bg,
    &lsh_kill,
    &lsh_source,
    &lsh_source
};

int lsh_num_builtins(void)
{
    return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
    const char *target = args[1];
    if (target == NULL || strcmp(target, "~") == 0)
    {
        target = getenv("HOME");
        if (target == NULL)
        {
            fprintf(stderr, "lsh: cd: HOME not set\n");
            return 1;
        }
    }
    if (chdir(target) != 0)
    {
        perror("lsh: cd");
        return 1;
    }
    return 0;
}

int lsh_pwd(char **args)
{
    (void)args;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
        return 0;
    }
    else
    {
        perror("lsh: pwd");
        return 1;
    }
}

int lsh_help(char **args)
{
    (void)args;
    printf("Apex Shell (apex-shell)\n");
    printf("A modern Unix shell with native observability & developer ergonomics.\n\n");
    printf("Built-in commands:\n");
    for (int i = 0; i < lsh_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }
    printf("\nFeatures supported:\n");
    printf("  - Safety Shield:    safemode [on|off|status] (blocks rm -rf / or dangerous targets)\n");
    printf("  - Aliases:          alias name='val', unalias name\n");
    printf("  - Job Control:      fg [%%id], bg [%%id], kill [-sig] pid|%%id, jobs\n");
    printf("  - Configuration:    source <file>, . <file> (loads profiles like ~/.apexrc)\n");
    printf("  - Observability:    time <cmd> (rusage profiler), sysinfo (proc dashboard)\n");
    printf("  - Intelligence:     Did-you-mean suggestions on command typos\n");
    printf("  - Scripting:        apex-shell script.apex or apex-shell -c \"commands\"\n");
    printf("  - History:          history (list), history N (last N), history -c (clear)\n");
    printf("  - Command Chaining: ; (seq), && (and), || (or)\n");
    printf("  - Background Jobs:  & (async), jobs (list active and stopped)\n");
    printf("  - Environment:      export KEY=VALUE, unset KEY, env\n");
    printf("  - Pipelines:        cmd1 | cmd2 | ... | cmdN\n");
    printf("  - I/O Redirection:  < (input), > (output), >> (append)\n");
    printf("  - Expansions:       $VAR, ${VAR}, $?, $$, ~ (tilde path)\n");
    printf("  - Prompt:           Git-aware branch and ANSI styling\n");
    printf("  - Quoted strings:   \"double quotes\" (expanded) and 'single quotes' (literal)\n");
    printf("  - Signal Handling:  Ctrl+C (SIGINT) and Ctrl+Z (SIGTSTP) protection\n");
    return 0;
}

int lsh_exit(char **args)
{
    g_should_exit = 1;
    if (args[1] != NULL)
    {
        g_last_exit_status = atoi(args[1]);
    }
    return g_last_exit_status;
}

int lsh_export(char **args)
{
    if (args[1] == NULL)
    {
        return lsh_env(args);
    }
    for (int i = 1; args[i] != NULL; i++)
    {
        char *eq = strchr(args[i], '=');
        if (eq != NULL)
        {
            *eq = '\0';
            char *key = args[i];
            char *val = eq + 1;
            if (setenv(key, val, 1) != 0)
            {
                perror("lsh: export");
                return 1;
            }
        }
    }
    return 0;
}

int lsh_unset(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: unset: not enough arguments\n");
        return 1;
    }
    for (int i = 1; args[i] != NULL; i++)
    {
        if (unsetenv(args[i]) != 0)
        {
            perror("lsh: unset");
            return 1;
        }
    }
    return 0;
}

int lsh_env(char **args)
{
    (void)args;
    if (!environ)
    {
        return 0;
    }
    for (char **env = environ; *env != NULL; env++)
    {
        printf("%s\n", *env);
    }
    return 0;
}

int lsh_source(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "apex-shell: source: filename argument required\n");
        return 1;
    }

    char filepath[1024];
    if (args[1][0] == '~' && (args[1][1] == '/' || args[1][1] == '\0'))
    {
        const char *home = getenv("HOME");
        if (home)
        {
            snprintf(filepath, sizeof(filepath), "%s%s", home, args[1] + 1);
        }
        else
        {
            snprintf(filepath, sizeof(filepath), "%s", args[1]);
        }
    }
    else
    {
        snprintf(filepath, sizeof(filepath), "%s", args[1]);
    }

    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        perror("apex-shell: source");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;

    while (!g_should_exit && (read_bytes = getline(&line, &len, f)) != -1)
    {
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

