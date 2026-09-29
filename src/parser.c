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

static void append_tok_char(char **buf, size_t *len, size_t *cap, char c)
{
    if (*len + 1 >= *cap)
    {
        *cap *= 2;
        char *new_buf = realloc(*buf, *cap);
        if (!new_buf)
        {
            free(*buf);
            fprintf(stderr, "lsh: allocation error\n");
            exit(EXIT_FAILURE);
        }
        *buf = new_buf;
    }
    (*buf)[(*len)++] = c;
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

        if (strncmp(p, "2>&1", 4) == 0)
        {
            tok->text = strdup("2>&1");
            tok->is_literal = 0;
            p += 4;
        }
        else if (strncmp(p, "1>&2", 4) == 0)
        {
            tok->text = strdup("1>&2");
            tok->is_literal = 0;
            p += 4;
        }
        else if (strncmp(p, "2>>", 3) == 0)
        {
            tok->text = strdup("2>>");
            tok->is_literal = 0;
            p += 3;
        }
        else if (strncmp(p, "2>", 2) == 0)
        {
            tok->text = strdup("2>");
            tok->is_literal = 0;
            p += 2;
        }
        else if (strncmp(p, "&>", 2) == 0)
        {
            tok->text = strdup("&>");
            tok->is_literal = 0;
            p += 2;
        }
        else if (strncmp(p, "&&", 2) == 0 ||
                 strncmp(p, "||", 2) == 0 ||
                 strncmp(p, ">>", 2) == 0)
        {
            char s[3] = {p[0], p[1], '\0'};
            tok->text = strdup(s);
            tok->is_literal = 0;
            p += 2;
        }
        else if (strncmp(p, "<<<", 3) == 0)
        {
            tok->text = strdup("<<<");
            tok->is_literal = 0;
            p += 3;
        }
        else if (strncmp(p, "<<", 2) == 0)
        {
            tok->text = strdup("<<");
            tok->is_literal = 0;
            p += 2;
        }
        else if (*p == ';' || *p == '|' || *p == '<' || *p == '>' || *p == '&')
        {
            char s[2] = {*p, '\0'};
            tok->text = strdup(s);
            tok->is_literal = 0;
            p++;
        }
        else
        {
            int handled = 0;
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
                    tok->text = strndup(p + 1, (size_t)(q - (p + 1)));
                    tok->is_literal = 1;
                    p = q + 1;
                    handled = 1;
                }
            }

            if (!handled)
            {
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
                        append_tok_char(&buf, &len, &cap, *p++);
                        append_tok_char(&buf, &len, &cap, *p++);
                        int depth = 1;
                        while (*p && depth > 0)
                        {
                            if (*p == '(') depth++;
                            else if (*p == ')') depth--;
                            append_tok_char(&buf, &len, &cap, *p++);
                        }
                    }
                    else if (*p == '`')
                    {
                        append_tok_char(&buf, &len, &cap, *p++);
                        while (*p && *p != '`')
                        {
                            append_tok_char(&buf, &len, &cap, *p++);
                        }
                        if (*p == '`')
                        {
                            append_tok_char(&buf, &len, &cap, *p++);
                        }
                    }
                    else if (*p == '\'')
                    {
                        p++;
                        while (*p && *p != '\'')
                        {
                            append_tok_char(&buf, &len, &cap, *p++);
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
                            append_tok_char(&buf, &len, &cap, *p++);
                        }
                        if (*p == '"')
                        {
                            p++;
                        }
                    }
                    else
                    {
                        append_tok_char(&buf, &len, &cap, *p++);
                    }
                }
                buf[len] = '\0';
                tok->text = buf;
                tok->is_literal = 0;
            }
        }

        if (!tok->text)
        {
            fprintf(stderr, "lsh: allocation error\n");
            exit(EXIT_FAILURE);
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
