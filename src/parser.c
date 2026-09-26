#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "parser.h"
#include "signals.h"
#include "execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void lsh_print_prompt(void)
{
    if (!isatty(STDIN_FILENO))
    {
        return;
    }

    char cwd[1024];
    char hostname[1024];
    char *user = getenv("USER");
    if (!user)
    {
        user = "user";
    }

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strncpy(hostname, "localhost", sizeof(hostname));
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char *home = getenv("HOME");
        char display_cwd[1024];
        if (home && strncmp(cwd, home, strlen(home)) == 0)
        {
            snprintf(display_cwd, sizeof(display_cwd), "~%s", cwd + strlen(home));
        }
        else
        {
            snprintf(display_cwd, sizeof(display_cwd), "%s", cwd);
        }
        // Bold green for user@host, bold blue for cwd
        printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", user, hostname, display_cwd);
    }
    else
    {
        printf("> ");
    }
    fflush(stdout);
}

char *expand_token(const ShellToken *tok)
{
    if (tok->is_literal)
    {
        return strdup(tok->text);
    }

    if (strcmp(tok->text, "$?") == 0)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", g_last_exit_status);
        return strdup(buf);
    }

    if (tok->text[0] == '$' && tok->text[1] != '\0')
    {
        char *val = getenv(tok->text + 1);
        if (val)
        {
            return strdup(val);
        }
        return strdup("");
    }

    return strdup(tok->text);
}

#define LSH_TOK_BUFSIZE 64
ShellToken **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    ShellToken **tokens = malloc(bufsize * sizeof(ShellToken *));
    char *p = line;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (*p != '\0')
    {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        {
            p++;
        }
        if (*p == '\0')
        {
            break;
        }

        ShellToken *tok = malloc(sizeof(ShellToken));
        if (!tok)
        {
            fprintf(stderr, "lsh: allocation error\n");
            exit(EXIT_FAILURE);
        }

        if (*p == '"' || *p == '\'')
        {
            char quote = *p;
            p++;
            char *start = p;
            while (*p && *p != quote)
            {
                p++;
            }
            size_t len = p - start;
            tok->text = malloc(len + 1);
            memcpy(tok->text, start, len);
            tok->text[len] = '\0';
            tok->is_literal = (quote == '\'');
            if (*p == quote)
            {
                p++;
            }
        }
        else if ((*p == '&' && *(p + 1) == '&') ||
                 (*p == '|' && *(p + 1) == '|') ||
                 (*p == '>' && *(p + 1) == '>'))
        {
            tok->text = malloc(3);
            tok->text[0] = *p;
            tok->text[1] = *(p + 1);
            tok->text[2] = '\0';
            tok->is_literal = 0;
            p += 2;
        }
        else if (*p == ';' || *p == '|' || *p == '<' || *p == '>' || *p == '&')
        {
            tok->text = malloc(2);
            tok->text[0] = *p;
            tok->text[1] = '\0';
            tok->is_literal = 0;
            p++;
        }
        else
        {
            char *start = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' &&
                   *p != ';' && *p != '|' && *p != '&' && *p != '<' && *p != '>' &&
                   *p != '"' && *p != '\'')
            {
                p++;
            }
            size_t len = p - start;
            tok->text = malloc(len + 1);
            memcpy(tok->text, start, len);
            tok->text[len] = '\0';
            tok->is_literal = 0;
        }

        tokens[position++] = tok;

        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(ShellToken *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    tokens[position] = NULL;
    return tokens;
}

void lsh_free_tokens(ShellToken **tokens)
{
    if (!tokens)
    {
        return;
    }
    for (int i = 0; tokens[i] != NULL; i++)
    {
        free(tokens[i]->text);
        free(tokens[i]);
    }
    free(tokens);
}

#define LSH_RL_BUFSIZE 1024
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        if (g_interrupted || (c == EOF && errno == EINTR))
        {
            g_interrupted = 0;
            errno = 0;
            clearerr(stdin);
            free(buffer);
            char *empty = malloc(1);
            if (empty)
            {
                empty[0] = '\0';
            }
            return empty;
        }

        if (c == EOF)
        {
            if (position == 0)
            {
                free(buffer);
                return NULL;
            }
            buffer[position] = '\0';
            return buffer;
        }
        else if (c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = (char)c;
        }
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}
