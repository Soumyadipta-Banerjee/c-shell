#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "redirection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void handle_redirection(char **args)
{
    int i = 0, j = 0;

    while (args[i] != NULL)
    {
        if (strcmp(args[i], "<") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            int in_fd = open(args[i + 1], O_RDONLY);
            if (in_fd < 0)
            {
                perror("lsh: input redirection");
                exit(EXIT_FAILURE);
            }
            if (dup2(in_fd, STDIN_FILENO) < 0)
            {
                perror("lsh: dup2 input");
                exit(EXIT_FAILURE);
            }
            close(in_fd);
            i += 2;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            int out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0)
            {
                perror("lsh: output redirection");
                exit(EXIT_FAILURE);
            }
            if (dup2(out_fd, STDOUT_FILENO) < 0)
            {
                perror("lsh: dup2 output");
                exit(EXIT_FAILURE);
            }
            close(out_fd);
            i += 2;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            int out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (out_fd < 0)
            {
                perror("lsh: output redirection");
                exit(EXIT_FAILURE);
            }
            if (dup2(out_fd, STDOUT_FILENO) < 0)
            {
                perror("lsh: dup2 append");
                exit(EXIT_FAILURE);
            }
            close(out_fd);
            i += 2;
        }
        else if (strcmp(args[i], "2>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            int err_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (err_fd < 0)
            {
                perror("lsh: stderr redirection");
                exit(EXIT_FAILURE);
            }
            if (dup2(err_fd, STDERR_FILENO) < 0)
            {
                perror("lsh: dup2 stderr");
                exit(EXIT_FAILURE);
            }
            close(err_fd);
            i += 2;
        }
        else if (strcmp(args[i], "2>>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            int err_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (err_fd < 0)
            {
                perror("lsh: stderr append redirection");
                exit(EXIT_FAILURE);
            }
            if (dup2(err_fd, STDERR_FILENO) < 0)
            {
                perror("lsh: dup2 stderr append");
                exit(EXIT_FAILURE);
            }
            close(err_fd);
            i += 2;
        }
        else if (strcmp(args[i], "&>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            int all_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (all_fd < 0)
            {
                perror("lsh: all redirection");
                exit(EXIT_FAILURE);
            }
            if (dup2(all_fd, STDOUT_FILENO) < 0 || dup2(all_fd, STDERR_FILENO) < 0)
            {
                perror("lsh: dup2 all");
                exit(EXIT_FAILURE);
            }
            close(all_fd);
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
