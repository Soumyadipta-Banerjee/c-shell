#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "parser.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

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

        if (*p == '2' && *(p + 1) == '>' && *(p + 2) == '&' && *(p + 3) == '1')
        {
            tok->text = malloc(5);
            memcpy(tok->text, "2>&1", 5);
            tok->is_literal = 0;
            p += 4;
        }
        else if (*p == '1' && *(p + 1) == '>' && *(p + 2) == '&' && *(p + 3) == '2')
        {
            tok->text = malloc(5);
            memcpy(tok->text, "1>&2", 5);
            tok->is_literal = 0;
            p += 4;
        }
        else if (*p == '2' && *(p + 1) == '>' && *(p + 2) == '>')
        {
            tok->text = malloc(4);
            memcpy(tok->text, "2>>", 4);
            tok->is_literal = 0;
            p += 3;
        }
        else if (*p == '2' && *(p + 1) == '>')
        {
            tok->text = malloc(3);
            memcpy(tok->text, "2>", 3);
            tok->is_literal = 0;
            p += 2;
        }
        else if (*p == '&' && *(p + 1) == '>')
        {
            tok->text = malloc(3);
            memcpy(tok->text, "&>", 3);
            tok->is_literal = 0;
            p += 2;
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
        else if (*p == '<' && *(p + 1) == '<' && *(p + 2) == '<')
        {
            tok->text = malloc(4);
            memcpy(tok->text, "<<<", 4);
            tok->is_literal = 0;
            p += 3;
        }
        else if (*p == '<' && *(p + 1) == '<')
        {
            tok->text = malloc(3);
            memcpy(tok->text, "<<", 3);
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
            // Check if the entire token is wrapped in single quotes: 'literal'
            if (*p == '\'')
            {
                char *q = p + 1;
                while (*q && *q != '\'')
                {
                    q++;
                }
                if (*q == '\'' && (q[1] == ' ' || q[1] == '\t' || q[1] == '\r' || q[1] == '\n' ||
                                   q[1] == '\0' || q[1] == ';' || q[1] == '|' || q[1] == '&' ||
                                   q[1] == '<' || q[1] == '>'))
                {
                    size_t qlen = q - (p + 1);
                    tok->text = malloc(qlen + 1);
                    memcpy(tok->text, p + 1, qlen);
                    tok->text[qlen] = '\0';
                    tok->is_literal = 1;
                    p = q + 1;
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
                    continue;
                }
            }

            size_t cap = 64;
            size_t len = 0;
            char *buf = malloc(cap);
            if (!buf)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }

            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' &&
                   *p != ';' && *p != '|' && *p != '&' && *p != '<' && *p != '>')
            {
                if (*p == '$' && *(p + 1) == '(')
                {
                    if (len + 2 >= cap)
                    {
                        cap *= 2;
                        buf = realloc(buf, cap);
                    }
                    buf[len++] = *p++;
                    buf[len++] = *p++;
                    int depth = 1;
                    while (*p && depth > 0)
                    {
                        if (*p == '(') depth++;
                        else if (*p == ')') depth--;
                        if (len + 1 >= cap)
                        {
                            cap *= 2;
                            buf = realloc(buf, cap);
                        }
                        buf[len++] = *p++;
                    }
                }
                else if (*p == '`')
                {
                    if (len + 1 >= cap)
                    {
                        cap *= 2;
                        buf = realloc(buf, cap);
                    }
                    buf[len++] = *p++;
                    while (*p && *p != '`')
                    {
                        if (len + 1 >= cap)
                        {
                            cap *= 2;
                            buf = realloc(buf, cap);
                        }
                        buf[len++] = *p++;
                    }
                    if (*p == '`')
                    {
                        if (len + 1 >= cap)
                        {
                            cap *= 2;
                            buf = realloc(buf, cap);
                        }
                        buf[len++] = *p++;
                    }
                }
                else if (*p == '\'')
                {
                    p++;
                    while (*p && *p != '\'')
                    {
                        if (len + 1 >= cap)
                        {
                            cap *= 2;
                            buf = realloc(buf, cap);
                        }
                        buf[len++] = *p++;
                    }
                    if (*p == '\'')
                    {
                        p++;
                    }
                }
                else if (*p == '"')
                {
                    p++;
                    while (*p && *p != '"')
                    {
                        if (len + 1 >= cap)
                        {
                            cap *= 2;
                            buf = realloc(buf, cap);
                        }
                        buf[len++] = *p++;
                    }
                    if (*p == '"')
                    {
                        p++;
                    }
                }
                else
                {
                    if (len + 1 >= cap)
                    {
                        cap *= 2;
                        buf = realloc(buf, cap);
                    }
                    buf[len++] = *p++;
                }
            }
            buf[len] = '\0';
            tok->text = buf;
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
