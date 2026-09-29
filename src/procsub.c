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
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

typedef struct ProcSub {
    int fd;
    pid_t pid;
    struct ProcSub *next;
} ProcSub;

static ProcSub *s_procsubs = NULL;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} DynBuf;

static int dynbuf_init(DynBuf *b, size_t initial_cap)
{
    b->cap = initial_cap > 0 ? initial_cap : 64;
    b->len = 0;
    b->data = malloc(b->cap);
    if (!b->data) return 0;
    b->data[0] = '\0';
    return 1;
}

static int dynbuf_ensure(DynBuf *b, size_t needed)
{
    if (b->len + needed < b->cap) return 1;
    size_t new_cap = b->cap * 2;
    while (b->len + needed >= new_cap) new_cap *= 2;
    char *new_data = realloc(b->data, new_cap);
    if (!new_data) return 0;
    b->data = new_data;
    b->cap = new_cap;
    return 1;
}

static int dynbuf_putc(DynBuf *b, char c)
{
    if (!dynbuf_ensure(b, 2)) return 0;
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
    return 1;
}

static int dynbuf_puts(DynBuf *b, const char *s, size_t n)
{
    if (!dynbuf_ensure(b, n + 1)) return 0;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 1;
}

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
            curr->fd = -1;
        }
        int status = 0;
        while (waitpid(curr->pid, &status, 0) < 0)
        {
            if (errno != EINTR) break;
        }
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

    DynBuf out;
    if (!dynbuf_init(&out, strlen(line) + 128))
    {
        return strdup(line);
    }

    int in_single_quote = 0;
    int in_double_quote = 0;

    for (size_t i = 0; line[i] != '\0';)
    {
        if (line[i] == '\\' && line[i + 1] != '\0' && !in_single_quote)
        {
            if (!dynbuf_putc(&out, line[i++]) || !dynbuf_putc(&out, line[i++]))
            {
                free(out.data);
                reap_process_substitutions();
                return strdup(line);
            }
            continue;
        }

        if (line[i] == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            if (!dynbuf_putc(&out, line[i++]))
            {
                free(out.data);
                reap_process_substitutions();
                return strdup(line);
            }
            continue;
        }

        if (line[i] == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            if (!dynbuf_putc(&out, line[i++]))
            {
                free(out.data);
                reap_process_substitutions();
                return strdup(line);
            }
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
                            // In child subshell: restore default signal handling
                            signal(SIGINT, SIG_DFL);
                            signal(SIGQUIT, SIG_DFL);
                            signal(SIGTSTP, SIG_DFL);
                            signal(SIGPIPE, SIG_DFL);
                            signal(SIGTTIN, SIG_DFL);
                            signal(SIGTTOU, SIG_DFL);

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
                            free(out.data);
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

                            if (!dynbuf_puts(&out, dev_fd_path, path_len))
                            {
                                free(out.data);
                                free(cmd_str);
                                reap_process_substitutions();
                                return strdup(line);
                            }

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

        if (!dynbuf_putc(&out, line[i++]))
        {
            free(out.data);
            reap_process_substitutions();
            return strdup(line);
        }
    }

    return out.data;
}

