#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "expander.h"
#include "parser.h"
#include "execute.h"
#include "arithmetic.h"
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

static char *capture_command_output(const char *cmd)
{
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        return strdup("");
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);

        ShellToken **tokens = lsh_split_line((char *)cmd);
        if (tokens)
        {
            lsh_execute_line(tokens);
            lsh_free_tokens(tokens);
        }
        exit(g_last_exit_status);
    }
    else if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        return strdup("");
    }

    close(pipefd[1]);
    size_t cap = 256;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf)
    {
        close(pipefd[0]);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR);
        return strdup("");
    }

    char chunk[256];
    ssize_t bytes;
    while ((bytes = read(pipefd[0], chunk, sizeof(chunk))) > 0)
    {
        while (len + bytes + 1 > cap)
        {
            cap *= 2;
            char *new_buf = realloc(buf, cap);
            if (!new_buf)
            {
                break;
            }
            buf = new_buf;
        }
        memcpy(buf + len, chunk, bytes);
        len += bytes;
    }
    close(pipefd[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno != EINTR)
        {
            break;
        }
    }
    if (WIFEXITED(status))
    {
        g_last_exit_status = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        g_last_exit_status = 128 + WTERMSIG(status);
    }

    // Strip trailing newlines (standard POSIX command substitution)
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    {
        len--;
    }
    buf[len] = '\0';
    return buf;
}

static char *expand_variables_in_string(const char *str)
{
    size_t cap = strlen(str) + 64;
    size_t len = 0;
    char *out = malloc(cap);
    if (!out)
    {
        return NULL;
    }

    const char *p = str;
    while (*p)
    {
        if (*p == '$' && *(p + 1) == '(' && *(p + 2) == '(')
        {
            p += 3; // skip "$(("
            const char *start = p;
            int depth = 1;
            while (*p)
            {
                if (*p == '(')
                {
                    depth++;
                }
                else if (*p == ')')
                {
                    if (depth == 1 && *(p + 1) == ')')
                    {
                        break;
                    }
                    depth--;
                }
                p++;
            }
            size_t expr_len = p - start;
            char expr[1024];
            if (expr_len < sizeof(expr))
            {
                strncpy(expr, start, expr_len);
                expr[expr_len] = '\0';
                int err = 0;
                long long val = evaluate_arithmetic_expression(expr, &err);
                char num_buf[32];
                snprintf(num_buf, sizeof(num_buf), "%lld", val);
                size_t nlen = strlen(num_buf);
                while (len + nlen + 1 > cap)
                {
                    cap *= 2;
                    char *new_out = realloc(out, cap);
                    if (!new_out) { free(out); return NULL; }
                    out = new_out;
                }
                memcpy(out + len, num_buf, nlen);
                len += nlen;
            }
            if (*p == ')') p++;
            if (*p == ')') p++;
        }
        else if (*p == '$' && *(p + 1) == '(')
        {
            p += 2; // skip "$("
            const char *start = p;
            int depth = 1;
            while (*p && depth > 0)
            {
                if (*p == '(') depth++;
                else if (*p == ')') depth--;
                if (depth > 0) p++;
            }
            size_t subcmd_len = p - start;
            char subcmd[1024];
            if (subcmd_len < sizeof(subcmd))
            {
                strncpy(subcmd, start, subcmd_len);
                subcmd[subcmd_len] = '\0';
                char *sub_out = capture_command_output(subcmd);
                if (sub_out)
                {
                    size_t solen = strlen(sub_out);
                    while (len + solen + 1 > cap)
                    {
                        cap *= 2;
                        char *new_out = realloc(out, cap);
                        if (!new_out) { free(sub_out); free(out); return NULL; }
                        out = new_out;
                    }
                    memcpy(out + len, sub_out, solen);
                    len += solen;
                    free(sub_out);
                }
            }
            if (*p == ')') p++;
        }
        else if (*p == '`')
        {
            p++; // skip '`'
            const char *start = p;
            while (*p && *p != '`')
            {
                p++;
            }
            size_t subcmd_len = p - start;
            char subcmd[1024];
            if (subcmd_len < sizeof(subcmd))
            {
                strncpy(subcmd, start, subcmd_len);
                subcmd[subcmd_len] = '\0';
                char *sub_out = capture_command_output(subcmd);
                if (sub_out)
                {
                    size_t solen = strlen(sub_out);
                    while (len + solen + 1 > cap)
                    {
                        cap *= 2;
                        char *new_out = realloc(out, cap);
                        if (!new_out) { free(sub_out); free(out); return NULL; }
                        out = new_out;
                    }
                    memcpy(out + len, sub_out, solen);
                    len += solen;
                    free(sub_out);
                }
            }
            if (*p == '`') p++;
        }
        else if (*p == '$')
        {
            p++;
            if (*p == '?')
            {
                p++;
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", g_last_exit_status);
                size_t blen = strlen(buf);
                while (len + blen + 1 > cap)
                {
                    cap *= 2;
                    char *new_out = realloc(out, cap);
                    if (!new_out) { free(out); return NULL; }
                    out = new_out;
                }
                memcpy(out + len, buf, blen);
                len += blen;
            }
            else if (*p == '$')
            {
                p++;
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", (int)getpid());
                size_t blen = strlen(buf);
                while (len + blen + 1 > cap)
                {
                    cap *= 2;
                    char *new_out = realloc(out, cap);
                    if (!new_out) { free(out); return NULL; }
                    out = new_out;
                }
                memcpy(out + len, buf, blen);
                len += blen;
            }
            else if (*p == '{')
            {
                p++; // skip '{'
                const char *start = p;
                while (*p && *p != '}')
                {
                    p++;
                }
                size_t var_len = p - start;
                char var_name[256];
                if (var_len < sizeof(var_name))
                {
                    strncpy(var_name, start, var_len);
                    var_name[var_len] = '\0';
                    char *val = getenv(var_name);
                    if (val)
                    {
                        size_t vlen = strlen(val);
                        while (len + vlen + 1 > cap)
                        {
                            cap *= 2;
                            char *new_out = realloc(out, cap);
                            if (!new_out) { free(out); return NULL; }
                            out = new_out;
                        }
                        memcpy(out + len, val, vlen);
                        len += vlen;
                    }
                    else if (g_shell_opts.nounset)
                    {
                        fprintf(stderr, "apex-shell: %s: unbound variable\n", var_name);
                        g_last_exit_status = 1;
                    }
                }
                if (*p == '}')
                {
                    p++; // skip '}'
                }
            }
            else if (isalpha((unsigned char)*p) || *p == '_')
            {
                const char *start = p;
                while (isalnum((unsigned char)*p) || *p == '_')
                {
                    p++;
                }
                size_t var_len = p - start;
                char var_name[256];
                if (var_len < sizeof(var_name))
                {
                    strncpy(var_name, start, var_len);
                    var_name[var_len] = '\0';
                    char *val = getenv(var_name);
                    if (val)
                    {
                        size_t vlen = strlen(val);
                        while (len + vlen + 1 > cap)
                        {
                            cap *= 2;
                            char *new_out = realloc(out, cap);
                            if (!new_out) { free(out); return NULL; }
                            out = new_out;
                        }
                        memcpy(out + len, val, vlen);
                        len += vlen;
                    }
                    else if (g_shell_opts.nounset)
                    {
                        fprintf(stderr, "apex-shell: %s: unbound variable\n", var_name);
                        g_last_exit_status = 1;
                    }
                }
            }
            else
            {
                // Standalone '$' without a variable name
                if (len + 2 > cap)
                {
                    cap *= 2;
                    char *new_out = realloc(out, cap);
                    if (!new_out) { free(out); return NULL; }
                    out = new_out;
                }
                out[len++] = '$';
            }
        }
        else
        {
            if (len + 2 > cap)
            {
                cap *= 2;
                char *new_out = realloc(out, cap);
                if (!new_out) { free(out); return NULL; }
                out = new_out;
            }
            out[len++] = *p++;
        }
    }
    out[len] = '\0';
    return out;
}

char *expand_token(const ShellToken *tok)
{
    if (tok->is_literal)
    {
        return strdup(tok->text);
    }

    char *intermediate = NULL;
    // Tilde expansion: ~ or ~/path
    if (tok->text[0] == '~' && (tok->text[1] == '/' || tok->text[1] == '\0'))
    {
        char *home = getenv("HOME");
        if (home)
        {
            size_t home_len = strlen(home);
            size_t rest_len = strlen(tok->text + 1);
            intermediate = malloc(home_len + rest_len + 1);
            if (intermediate)
            {
                strcpy(intermediate, home);
                strcat(intermediate, tok->text + 1);
            }
        }
    }

    if (!intermediate)
    {
        intermediate = strdup(tok->text);
    }

    char *expanded = expand_variables_in_string(intermediate);
    free(intermediate);
    return expanded ? expanded : strdup("");
}
