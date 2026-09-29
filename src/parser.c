#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "parser.h"
#include "signals.h"
#include "execute.h"
#include "arithmetic.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

static char *get_git_branch(void)
{
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        return NULL;
    }

    char dir[1024];
    strncpy(dir, cwd, sizeof(dir));

    while (1)
    {
        char head_path[1200];
        snprintf(head_path, sizeof(head_path), "%s/.git/HEAD", dir);

        FILE *f = fopen(head_path, "r");
        if (f)
        {
            char line[256];
            if (fgets(line, sizeof(line), f))
            {
                fclose(f);
                line[strcspn(line, "\r\n")] = '\0';
                const char *ref_prefix = "ref: refs/heads/";
                if (strncmp(line, ref_prefix, strlen(ref_prefix)) == 0)
                {
                    return strdup(line + strlen(ref_prefix));
                }
                else
                {
                    char short_hash[8];
                    strncpy(short_hash, line, 7);
                    short_hash[7] = '\0';
                    return strdup(short_hash);
                }
            }
            fclose(f);
        }

        if (strcmp(dir, "/") == 0)
        {
            break;
        }
        char *last_slash = strrchr(dir, '/');
        if (!last_slash || last_slash == dir)
        {
            strcpy(dir, "/");
        }
        else
        {
            *last_slash = '\0';
        }
    }
    return NULL;
}

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

        char *git_branch = get_git_branch();
        if (git_branch)
        {
            // Bold green for user@host, bold blue for cwd, bold cyan for git branch
            printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m \033[1;36m(git:%s)\033[0m$ ",
                   user, hostname, display_cwd, git_branch);
            free(git_branch);
        }
        else
        {
            printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", user, hostname, display_cwd);
        }
    }
    else
    {
        printf("> ");
    }
    fflush(stdout);
}

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
        waitpid(pid, NULL, 0);
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
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
    {
        g_last_exit_status = WEXITSTATUS(status);
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
