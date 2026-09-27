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
void lsh_record_frecency(const char *path);

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
    ".",
    "pushd",
    "popd",
    "dirs",
    "z"
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
    &lsh_source,
    &lsh_pushd,
    &lsh_popd,
    &lsh_dirs,
    &lsh_z
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
    lsh_record_frecency(target);
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
    printf("  - Navigation:       pushd <dir>, popd, dirs, z <query> (frecency jump)\n");
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

#define LSH_DIR_STACK_MAX 64
static char *s_dir_stack[LSH_DIR_STACK_MAX];
static int s_dir_stack_count = 0;

typedef struct {
    char *path;
    int score;
} FrecencyEntry;

#define MAX_FRECENCY 128
static FrecencyEntry s_frecency[MAX_FRECENCY];
static int s_frecency_count = 0;

void lsh_record_frecency(const char *path)
{
    if (!path || path[0] == '\0') return;
    char resolved[1024];
    if (realpath(path, resolved) == NULL)
    {
        strncpy(resolved, path, sizeof(resolved) - 1);
        resolved[sizeof(resolved) - 1] = '\0';
    }

    for (int i = 0; i < s_frecency_count; i++)
    {
        if (strcmp(s_frecency[i].path, resolved) == 0)
        {
            s_frecency[i].score += 5;
            return;
        }
    }

    if (s_frecency_count < MAX_FRECENCY)
    {
        s_frecency[s_frecency_count].path = strdup(resolved);
        s_frecency[s_frecency_count].score = 10;
        s_frecency_count++;
    }
    else
    {
        int min_idx = 0;
        for (int i = 1; i < s_frecency_count; i++)
        {
            if (s_frecency[i].score < s_frecency[min_idx].score)
            {
                min_idx = i;
            }
        }
        free(s_frecency[min_idx].path);
        s_frecency[min_idx].path = strdup(resolved);
        s_frecency[min_idx].score = 10;
    }
}

int lsh_dirs(char **args)
{
    (void)args;
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("apex-shell: dirs");
        return 1;
    }
    printf("%s", cwd);
    for (int i = s_dir_stack_count - 1; i >= 0; i--)
    {
        printf(" %s", s_dir_stack[i]);
    }
    printf("\n");
    fflush(stdout);
    return 0;
}

int lsh_pushd(char **args)
{
    const char *target = args[1];
    if (!target)
    {
        fprintf(stderr, "apex-shell: pushd: no other directory\n");
        return 1;
    }

    if (s_dir_stack_count >= LSH_DIR_STACK_MAX)
    {
        fprintf(stderr, "apex-shell: pushd: directory stack full\n");
        return 1;
    }

    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("apex-shell: pushd");
        return 1;
    }

    char resolved[1024];
    if (target[0] == '~' && (target[1] == '/' || target[1] == '\0'))
    {
        const char *home = getenv("HOME");
        if (home)
        {
            snprintf(resolved, sizeof(resolved), "%s%s", home, target + 1);
        }
        else
        {
            snprintf(resolved, sizeof(resolved), "%s", target);
        }
    }
    else
    {
        snprintf(resolved, sizeof(resolved), "%s", target);
    }

    if (chdir(resolved) != 0)
    {
        perror("apex-shell: pushd");
        return 1;
    }

    s_dir_stack[s_dir_stack_count++] = strdup(cwd);
    lsh_record_frecency(resolved);
    return lsh_dirs(NULL);
}

int lsh_popd(char **args)
{
    (void)args;
    if (s_dir_stack_count <= 0)
    {
        fprintf(stderr, "apex-shell: popd: directory stack empty\n");
        return 1;
    }

    char *target = s_dir_stack[--s_dir_stack_count];
    if (chdir(target) != 0)
    {
        perror("apex-shell: popd");
        free(target);
        return 1;
    }

    lsh_record_frecency(target);
    free(target);
    return lsh_dirs(NULL);
}

int lsh_z(char **args)
{
    if (args[1] == NULL)
    {
        for (int i = 0; i < s_frecency_count; i++)
        {
            printf("%4d  %s\n", s_frecency[i].score, s_frecency[i].path);
        }
        fflush(stdout);
        return 0;
    }

    const char *query = args[1];
    int best_idx = -1;
    int best_score = -1;

    for (int i = 0; i < s_frecency_count; i++)
    {
        if (strstr(s_frecency[i].path, query) != NULL)
        {
            if (s_frecency[i].score > best_score)
            {
                best_score = s_frecency[i].score;
                best_idx = i;
            }
        }
    }

    if (best_idx == -1)
    {
        fprintf(stderr, "apex-shell: z: no matching directory for '%s'\n", query);
        return 1;
    }

    if (chdir(s_frecency[best_idx].path) != 0)
    {
        perror("apex-shell: z");
        return 1;
    }

    s_frecency[best_idx].score += 5;
    printf("%s\n", s_frecency[best_idx].path);
    fflush(stdout);
    return 0;
}

