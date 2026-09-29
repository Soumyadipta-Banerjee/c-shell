#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

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

static void append_str(char *out, size_t *len, size_t cap, const char *str)
{
    if (!str) return;
    while (*str && *len + 1 < cap)
    {
        out[(*len)++] = *str++;
    }
    out[*len] = '\0';
}

static void append_char(char *out, size_t *len, size_t cap, char c)
{
    if (*len + 1 < cap)
    {
        out[(*len)++] = c;
        out[*len] = '\0';
    }
}

int format_ps1(const char *ps1, char *out, size_t out_cap)
{
    if (!ps1 || !out || out_cap == 0) return -1;

    char cwd[1024];
    char hostname[1024];
    char *user = getenv("USER");
    if (!user) user = "user";

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strncpy(hostname, "localhost", sizeof(hostname));
    }

    char display_cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char *home = getenv("HOME");
        if (home && strncmp(cwd, home, strlen(home)) == 0)
        {
            snprintf(display_cwd, sizeof(display_cwd), "~%s", cwd + strlen(home));
        }
        else
        {
            snprintf(display_cwd, sizeof(display_cwd), "%s", cwd);
        }
    }
    else
    {
        strncpy(display_cwd, "/", sizeof(display_cwd));
    }

    char *git_branch = get_git_branch();
    size_t len = 0;
    out[0] = '\0';

    const char *p = ps1;
    while (*p && len + 1 < out_cap)
    {
        if (*p == '\\')
        {
            p++;
            if (*p == '\0') break;
            switch (*p)
            {
            case 'u':
                append_str(out, &len, out_cap, user);
                break;
            case 'h':
            {
                const char *dot = strchr(hostname, '.');
                if (dot)
                {
                    char short_host[256];
                    size_t hlen = (size_t)(dot - hostname);
                    if (hlen >= sizeof(short_host)) hlen = sizeof(short_host) - 1;
                    strncpy(short_host, hostname, hlen);
                    short_host[hlen] = '\0';
                    append_str(out, &len, out_cap, short_host);
                }
                else
                {
                    append_str(out, &len, out_cap, hostname);
                }
                break;
            }
            case 'H':
                append_str(out, &len, out_cap, hostname);
                break;
            case 'w':
                append_str(out, &len, out_cap, display_cwd);
                break;
            case 'W':
            {
                if (strcmp(display_cwd, "~") == 0 || strcmp(display_cwd, "/") == 0)
                {
                    append_str(out, &len, out_cap, display_cwd);
                }
                else
                {
                    const char *last = strrchr(display_cwd, '/');
                    append_str(out, &len, out_cap, last ? last + 1 : display_cwd);
                }
                break;
            }
            case 't':
            {
                time_t now = time(NULL);
                struct tm *tm_info = localtime(&now);
                if (tm_info)
                {
                    char tbuf[32];
                    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", tm_info);
                    append_str(out, &len, out_cap, tbuf);
                }
                break;
            }
            case 'd':
            {
                time_t now = time(NULL);
                struct tm *tm_info = localtime(&now);
                if (tm_info)
                {
                    char dbuf[32];
                    strftime(dbuf, sizeof(dbuf), "%a %b %d", tm_info);
                    append_str(out, &len, out_cap, dbuf);
                }
                break;
            }
            case 'g':
                if (git_branch && *git_branch)
                {
                    append_str(out, &len, out_cap, git_branch);
                }
                break;
            case '$':
                append_str(out, &len, out_cap, (geteuid() == 0) ? "#" : "$");
                break;
            case 'e':
                append_char(out, &len, out_cap, '\033');
                break;
            case '0':
                if (p[1] == '3' && p[2] == '3')
                {
                    append_char(out, &len, out_cap, '\033');
                    p += 2;
                }
                else
                {
                    append_char(out, &len, out_cap, '0');
                }
                break;
            case 'n':
                append_char(out, &len, out_cap, '\n');
                break;
            case '\\':
                append_char(out, &len, out_cap, '\\');
                break;
            default:
                append_char(out, &len, out_cap, *p);
                break;
            }
            p++;
        }
        else
        {
            append_char(out, &len, out_cap, *p++);
        }
    }

    if (git_branch)
    {
        free(git_branch);
    }
    return 0;
}

void lsh_print_prompt(void)
{
    if (!isatty(STDIN_FILENO))
    {
        return;
    }

    char *ps1 = getenv("PS1");
    if (ps1 && *ps1 != '\0')
    {
        char prompt_buf[2048];
        if (format_ps1(ps1, prompt_buf, sizeof(prompt_buf)) == 0)
        {
            fputs(prompt_buf, stdout);
            fflush(stdout);
            return;
        }
    }

    char cwd[1024];
    char hostname[1024];
    char *user = getenv("USER");
    if (!user) user = "user";

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
