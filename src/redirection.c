#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "redirection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static void check_target_arg(const char *arg)
{
    if (arg == NULL)
    {
        fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
        exit(EXIT_FAILURE);
    }
}

static void redirect_file(const char *path, int target_fd, int flags, const char *err_label)
{
    int fd = open(path, flags, 0644);
    if (fd < 0)
    {
        perror(err_label);
        exit(EXIT_FAILURE);
    }
    if (dup2(fd, target_fd) < 0)
    {
        perror("lsh: dup2");
        exit(EXIT_FAILURE);
    }
    close(fd);
}

void handle_redirection(char **args)
{
    int i = 0, j = 0;

    while (args[i] != NULL)
    {
        if (strcmp(args[i], "<") == 0)
        {
            check_target_arg(args[i + 1]);
            redirect_file(args[i + 1], STDIN_FILENO, O_RDONLY, "lsh: input redirection");
            i += 2;
        }
        else if (strcmp(args[i], "<<<") == 0)
        {
            check_target_arg(args[i + 1]);
            int pfd[2];
            if (pipe(pfd) < 0)
            {
                perror("lsh: pipe");
                exit(EXIT_FAILURE);
            }
            size_t slen = strlen(args[i + 1]);
            ssize_t w1 = write(pfd[1], args[i + 1], slen);
            ssize_t w2 = write(pfd[1], "\n", 1);
            (void)w1;
            (void)w2;
            close(pfd[1]);
            if (dup2(pfd[0], STDIN_FILENO) < 0)
            {
                perror("lsh: dup2 <<<");
                exit(EXIT_FAILURE);
            }
            close(pfd[0]);
            i += 2;
        }
        else if (strcmp(args[i], "<<") == 0)
        {
            check_target_arg(args[i + 1]);
            const char *delim = args[i + 1];
            int pfd[2];
            if (pipe(pfd) < 0)
            {
                perror("lsh: pipe");
                exit(EXIT_FAILURE);
            }

            int in_dup = dup(STDIN_FILENO);
            FILE *stream = (in_dup >= 0) ? fdopen(in_dup, "r") : NULL;
            if (stream)
            {
                char *line = NULL;
                size_t cap = 0;
                ssize_t nread;
                int is_tty = isatty(STDERR_FILENO);

                if (is_tty)
                {
                    fprintf(stderr, "> ");
                    fflush(stderr);
                }

                while ((nread = getline(&line, &cap, stream)) != -1)
                {
                    while (nread > 0 && (line[nread - 1] == '\n' || line[nread - 1] == '\r'))
                    {
                        line[--nread] = '\0';
                    }
                    if (strcmp(line, delim) == 0)
                    {
                        break;
                    }
                    ssize_t w1 = write(pfd[1], line, nread);
                    ssize_t w2 = write(pfd[1], "\n", 1);
                    (void)w1;
                    (void)w2;
                    if (is_tty)
                    {
                        fprintf(stderr, "> ");
                        fflush(stderr);
                    }
                }
                free(line);
                fclose(stream);
            }

            close(pfd[1]);
            if (dup2(pfd[0], STDIN_FILENO) < 0)
            {
                perror("lsh: dup2 <<");
                exit(EXIT_FAILURE);
            }
            close(pfd[0]);
            i += 2;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            check_target_arg(args[i + 1]);
            redirect_file(args[i + 1], STDOUT_FILENO, O_WRONLY | O_CREAT | O_TRUNC, "lsh: output redirection");
            i += 2;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            check_target_arg(args[i + 1]);
            redirect_file(args[i + 1], STDOUT_FILENO, O_WRONLY | O_CREAT | O_APPEND, "lsh: output redirection");
            i += 2;
        }
        else if (strcmp(args[i], "2>") == 0)
        {
            check_target_arg(args[i + 1]);
            redirect_file(args[i + 1], STDERR_FILENO, O_WRONLY | O_CREAT | O_TRUNC, "lsh: stderr redirection");
            i += 2;
        }
        else if (strcmp(args[i], "2>>") == 0)
        {
            check_target_arg(args[i + 1]);
            redirect_file(args[i + 1], STDERR_FILENO, O_WRONLY | O_CREAT | O_APPEND, "lsh: stderr append redirection");
            i += 2;
        }
        else if (strcmp(args[i], "&>") == 0)
        {
            check_target_arg(args[i + 1]);
            redirect_file(args[i + 1], STDOUT_FILENO, O_WRONLY | O_CREAT | O_TRUNC, "lsh: all redirection");
            if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
            {
                perror("lsh: dup2 all");
                exit(EXIT_FAILURE);
            }
            i += 2;
        }
        else if (strcmp(args[i], "2>&1") == 0)
        {
            if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
            {
                perror("lsh: dup2 2>&1");
                exit(EXIT_FAILURE);
            }
            i += 1;
        }
        else if (strcmp(args[i], "1>&2") == 0)
        {
            if (dup2(STDERR_FILENO, STDOUT_FILENO) < 0)
            {
                perror("lsh: dup2 1>&2");
                exit(EXIT_FAILURE);
            }
            i += 1;
        }
        else
        {
            args[j++] = args[i++];
        }
    }
    args[j] = NULL;
}

int resolve_heredocs(ShellToken **tokens, char *(*read_line_cb)(void *ctx), void *ctx)
{
    if (!tokens || !read_line_cb) return 0;

    for (int i = 0; tokens[i] != NULL; i++)
    {
        if (strcmp(tokens[i]->text, "<<") == 0 && tokens[i + 1] != NULL)
        {
            char *delim = tokens[i + 1]->text;
            size_t cap = 512, len = 0;
            char *body = malloc(cap);
            if (!body) return -1;
            body[0] = '\0';

            char *line = NULL;
            while ((line = read_line_cb(ctx)) != NULL)
            {
                size_t llen = strlen(line);
                while (llen > 0 && (line[llen - 1] == '\r' || line[llen - 1] == '\n'))
                {
                    line[--llen] = '\0';
                }

                if (strcmp(line, delim) == 0)
                {
                    free(line);
                    break;
                }

                while (len + llen + 2 > cap)
                {
                    cap *= 2;
                    char *new_body = realloc(body, cap);
                    if (!new_body)
                    {
                        free(line);
                        free(body);
                        return -1;
                    }
                    body = new_body;
                }

                if (len > 0)
                {
                    body[len++] = '\n';
                }
                memcpy(body + len, line, llen);
                len += llen;
                body[len] = '\0';
                free(line);
            }

            free(tokens[i]->text);
            tokens[i]->text = strdup("<<<");

            free(tokens[i + 1]->text);
            tokens[i + 1]->text = body;
            tokens[i + 1]->is_literal = 1;
        }
    }
    return 0;
}
