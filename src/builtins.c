#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "builtins.h"
#include "execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char *builtin_str[] = {
    "cd",
    "pwd",
    "help",
    "exit"
};

int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_pwd,
    &lsh_help,
    &lsh_exit
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
    printf("Soumya's C-Shell\n");
    printf("Type program names and arguments, then hit enter.\n\n");
    printf("Built-in commands:\n");
    for (int i = 0; i < lsh_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }
    printf("\nFeatures supported:\n");
    printf("  - Command Chaining: ; (seq), && (and), || (or)\n");
    printf("  - Pipelines:        cmd1 | cmd2 | ... | cmdN\n");
    printf("  - I/O Redirection:  < (input), > (output), >> (append)\n");
    printf("  - Quoted strings:   \"hello world\" or 'hello world'\n");
    printf("  - Expansions:       $VAR, $?\n");
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
