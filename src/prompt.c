#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
