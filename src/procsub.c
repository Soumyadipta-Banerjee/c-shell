#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "procsub.h"
#include "parser.h"
#include "execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

typedef struct ProcSub {
    int fd;
    pid_t pid;
    struct ProcSub *next;
} ProcSub;

static ProcSub *s_procsubs = NULL;

int has_process_substitution(const char *line)
{
    if (!line) return 0;

    int in_single_quote = 0;
    int in_double_quote = 0;

    for (int i = 0; line[i] != '\0'; i++)
    {
        if (line[i] == '\\' && line[i + 1] != '\0' && !in_single_quote)
        {
            i++;
            continue;
        }

        if (line[i] == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            continue;
        }
        if (line[i] == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            continue;
        }

        if (!in_single_quote && (line[i] == '<' || line[i] == '>') && line[i + 1] == '(')
        {
            return 1;
        }
    }
    return 0;
}

static void register_procsub(int fd, pid_t pid)
{
    ProcSub *ps = malloc(sizeof(ProcSub));
    if (!ps) return;
    ps->fd = fd;
    ps->pid = pid;
    ps->next = s_procsubs;
    s_procsubs = ps;
}

void reap_process_substitutions(void)
{
    ProcSub *curr = s_procsubs;
    while (curr)
    {
        ProcSub *next = curr->next;
        if (curr->fd >= 0)
        {
            close(curr->fd);
        }
        int status = 0;
        waitpid(curr->pid, &status, 0);
        free(curr);
        curr = next;
    }
    s_procsubs = NULL;
}

char *resolve_process_substitutions(const char *line)
{
    if (!line) return NULL;
    if (!has_process_substitution(line))
    {
        return strdup(line);
    }

    size_t cap = strlen(line) + 128;
    size_t len = 0;
    char *out = malloc(cap);
    if (!out) return strdup(line);

    int in_single_quote = 0;
    int in_double_quote = 0;

    for (size_t i = 0; line[i] != '\0';)
    {
        if (line[i] == '\\' && line[i + 1] != '\0' && !in_single_quote)
        {
            if (len + 2 >= cap)
            {
                cap *= 2;
                char *new_out = realloc(out, cap);
                if (!new_out) { free(out); return strdup(line); }
                out = new_out;
            }
            out[len++] = line[i++];
            out[len++] = line[i++];
            continue;
        }

        if (line[i] == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            out[len++] = line[i++];
            continue;
        }

        if (line[i] == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            out[len++] = line[i++];
            continue;
        }

        if (!in_single_quote && (line[i] == '<' || line[i] == '>') && line[i + 1] == '(')
        {
            int is_input = (line[i] == '<');
            size_t start = i + 2;
            int depth = 1;
            size_t end = start;
            int inner_single = 0;
            int inner_double = 0;

            while (line[end] != '\0')
            {
                if (line[end] == '\\' && line[end + 1] != '\0' && !inner_single)
                {
                    end += 2;
                    continue;
                }
                if (line[end] == '\'' && !inner_double)
                {
                    inner_single = !inner_single;
                }
                else if (line[end] == '"' && !inner_single)
                {
                    inner_double = !inner_double;
                }
                else if (!inner_single && !inner_double)
                {
                    if (line[end] == '(')
                    {
                        depth++;
                    }
                    else if (line[end] == ')')
                    {
                        depth--;
                        if (depth == 0)
                        {
                            break;
                        }
                    }
                }
                end++;
            }

            if (line[end] == ')')
            {
                size_t cmd_len = end - start;
                char *cmd_str = malloc(cmd_len + 1);
                if (cmd_str)
                {
                    strncpy(cmd_str, line + start, cmd_len);
                    cmd_str[cmd_len] = '\0';

                    int pfd[2];
                    if (pipe(pfd) == 0)
                    {
                        pid_t pid = fork();
                        if (pid == 0)
                        {
                            // In child subshell
                            if (is_input)
                            {
                                close(pfd[0]);
                                dup2(pfd[1], STDOUT_FILENO);
                                close(pfd[1]);
                            }
                            else
                            {
                                close(pfd[1]);
                                dup2(pfd[0], STDIN_FILENO);
                                close(pfd[0]);
                            }

                            // Close any previously inherited procsub descriptors
                            ProcSub *ps = s_procsubs;
                            while (ps)
                            {
                                if (ps->fd >= 0) close(ps->fd);
                                ps = ps->next;
                            }

                            ShellToken **sub_tokens = lsh_split_line(cmd_str);
                            int rc = 0;
                            if (sub_tokens)
                            {
                                rc = lsh_execute_line(sub_tokens);
                                lsh_free_tokens(sub_tokens);
                            }
                            free(cmd_str);
                            free(out);
                            _exit(rc);
                        }
                        else if (pid > 0)
                        {
                            // In parent process
                            int tracked_fd = -1;
                            if (is_input)
                            {
                                close(pfd[1]); // Close write end
                                tracked_fd = pfd[0];
                            }
                            else
                            {
                                close(pfd[0]); // Close read end
                                tracked_fd = pfd[1];
                            }

                            register_procsub(tracked_fd, pid);

                            char dev_fd_path[32];
                            snprintf(dev_fd_path, sizeof(dev_fd_path), "/dev/fd/%d", tracked_fd);
                            size_t path_len = strlen(dev_fd_path);

                            while (len + path_len + 1 >= cap)
                            {
                                cap *= 2;
                                char *new_out = realloc(out, cap);
                                if (!new_out) { free(out); free(cmd_str); return strdup(line); }
                                out = new_out;
                            }

                            strcpy(out + len, dev_fd_path);
                            len += path_len;

                            i = end + 1; // Advance past ')'
                            free(cmd_str);
                            continue;
                        }
                        else
                        {
                            // Fork error: close pipe
                            close(pfd[0]);
                            close(pfd[1]);
                        }
                    }
                    free(cmd_str);
                }
            }
        }

        if (len + 2 >= cap)
        {
            cap *= 2;
            char *new_out = realloc(out, cap);
            if (!new_out) { free(out); return strdup(line); }
            out = new_out;
        }

        out[len++] = line[i++];
    }

    out[len] = '\0';
    return out;
}
