#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "highlight.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

static int is_builtin_command(const char *cmd)
{
    for (int i = 0; i < lsh_num_builtins(); i++)
    {
        if (strcmp(cmd, builtin_str[i]) == 0) return 1;
    }
    if (strcmp(cmd, "time") == 0) return 1;
    return 0;
}

static int is_executable_command(const char *cmd)
{
    if (cmd[0] == '\0') return 0;
    if (is_builtin_command(cmd)) return 1;
    if (strchr(cmd, '/'))
    {
        return access(cmd, X_OK) == 0;
    }

    char *path = getenv("PATH");
    if (!path) return 0;
    char *copy = strdup(path);
    if (!copy) return 0;

    char *saveptr = NULL;
    char *dir = strtok_r(copy, ":", &saveptr);
    int found = 0;
    while (dir)
    {
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", dir, cmd);
        if (access(full, X_OK) == 0)
        {
            found = 1;
            break;
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    free(copy);
    return found;
}

void print_syntax_highlighted(const char *buf, size_t len)
{
    size_t i = 0;
    int expecting_cmd = 1;

    while (i < len)
    {
        if (isspace((unsigned char)buf[i]))
        {
            putchar(buf[i++]);
            continue;
        }

        // Extended fd redirections: 2>&1, 1>&2, 2>>, 2>, &>
        if ((buf[i] == '2' && i + 1 < len && buf[i + 1] == '>') ||
            (buf[i] == '1' && i + 1 < len && buf[i + 1] == '>'))
        {
            printf("\033[1;35m"); // Bold Magenta
            while (i < len && !isspace((unsigned char)buf[i]) &&
                   (buf[i] == '1' || buf[i] == '2' || buf[i] == '>' || buf[i] == '&'))
            {
                putchar(buf[i++]);
            }
            printf("\033[0m");
            continue;
        }

        // Operators
        if (buf[i] == '|' || buf[i] == '&' || buf[i] == ';' || buf[i] == '<' || buf[i] == '>')
        {
            printf("\033[1;35m"); // Bold Magenta
            while (i < len && (buf[i] == '|' || buf[i] == '&' || buf[i] == ';' || buf[i] == '<' || buf[i] == '>'))
            {
                putchar(buf[i++]);
            }
            printf("\033[0m");
            expecting_cmd = 1;
            continue;
        }

        // Arithmetic expansion $(( ... ))
        if (buf[i] == '$' && i + 1 < len && buf[i + 1] == '(' && i + 2 < len && buf[i + 2] == '(')
        {
            printf("\033[1;36m"); // Bright Cyan
            while (i < len && buf[i] != ')')
            {
                putchar(buf[i++]);
            }
            if (i < len && buf[i] == ')') putchar(buf[i++]);
            if (i < len && buf[i] == ')') putchar(buf[i++]);
            printf("\033[0m");
            expecting_cmd = 0;
            continue;
        }

        // Single quoted string
        if (buf[i] == '\'')
        {
            printf("\033[36m"); // Cyan
            putchar(buf[i++]);
            while (i < len && buf[i] != '\'')
            {
                putchar(buf[i++]);
            }
            if (i < len && buf[i] == '\'')
            {
                putchar(buf[i++]);
            }
            printf("\033[0m");
            expecting_cmd = 0;
            continue;
        }

        // Double quoted string
        if (buf[i] == '"')
        {
            printf("\033[36m"); // Cyan
            putchar(buf[i++]);
            while (i < len && buf[i] != '"')
            {
                putchar(buf[i++]);
            }
            if (i < len && buf[i] == '"')
            {
                putchar(buf[i++]);
            }
            printf("\033[0m");
            expecting_cmd = 0;
            continue;
        }

        // Word (command, flag, variable, or argument)
        size_t start = i;
        while (i < len && !isspace((unsigned char)buf[i]) &&
               buf[i] != '|' && buf[i] != '&' && buf[i] != ';' &&
               buf[i] != '<' && buf[i] != '>' && buf[i] != '\'' && buf[i] != '"')
        {
            i++;
        }
        size_t wlen = i - start;
        char word[256];
        if (wlen < sizeof(word))
        {
            memcpy(word, buf + start, wlen);
            word[wlen] = '\0';
        }
        else
        {
            memcpy(word, buf + start, sizeof(word) - 1);
            word[sizeof(word) - 1] = '\0';
        }

        if (expecting_cmd)
        {
            if (is_executable_command(word))
            {
                printf("\033[1;32m%s\033[0m", word); // Bold Green
            }
            else
            {
                printf("\033[1;31m%s\033[0m", word); // Bold Red
            }
            expecting_cmd = 0;
        }
        else if (word[0] == '-')
        {
            printf("\033[33m%s\033[0m", word); // Yellow
        }
        else if (word[0] == '$')
        {
            printf("\033[1;34m%s\033[0m", word); // Bold Blue
        }
        else
        {
            printf("\033[0m%s", word); // Default
        }
    }
    printf("\033[0m");
}
