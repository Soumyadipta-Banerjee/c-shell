#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "globber.h"
#include "expander.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>

int has_glob_meta(const char *str)
{
    if (!str) return 0;
    while (*str)
    {
        if (*str == '*' || *str == '?' || *str == '[')
        {
            return 1;
        }
        str++;
    }
    return 0;
}

int expand_path_pattern(const char *pattern, char ***out_paths, int *out_count)
{
    if (!pattern || !out_paths || !out_count) return -1;

    glob_t g;
    int flags = GLOB_NOCHECK | GLOB_TILDE;
    int res = glob(pattern, flags, NULL, &g);
    if (res != 0)
    {
        *out_paths = NULL;
        *out_count = 0;
        return res;
    }

    char **paths = malloc((g.gl_pathc + 1) * sizeof(char *));
    if (!paths)
    {
        globfree(&g);
        *out_paths = NULL;
        *out_count = 0;
        return -1;
    }

    for (size_t i = 0; i < g.gl_pathc; i++)
    {
        paths[i] = strdup(g.gl_pathv[i]);
    }
    paths[g.gl_pathc] = NULL;
    *out_count = (int)g.gl_pathc;
    *out_paths = paths;

    globfree(&g);
    return 0;
}

void free_path_list(char **paths, int count)
{
    if (!paths) return;
    for (int i = 0; i < count; i++)
    {
        free(paths[i]);
    }
    free(paths);
}

char **expand_tokens_with_glob(ShellToken **tokens, int start, int len, int *out_count)
{
    int cap = len > 8 ? len * 2 : 16;
    int count = 0;
    char **args = malloc((cap + 1) * sizeof(char *));
    if (!args)
    {
        if (out_count) *out_count = 0;
        return NULL;
    }

    for (int j = 0; j < len; j++)
    {
        ShellToken *tok = tokens[start + j];
        char *expanded = expand_token(tok);
        if (!expanded)
        {
            continue;
        }

        if (!tok->is_literal && has_glob_meta(expanded))
        {
            char **paths = NULL;
            int pcount = 0;
            if (expand_path_pattern(expanded, &paths, &pcount) == 0 && paths)
            {
                for (int k = 0; k < pcount; k++)
                {
                    if (count + 1 >= cap)
                    {
                        cap *= 2;
                        char **new_args = realloc(args, (cap + 1) * sizeof(char *));
                        if (!new_args) break;
                        args = new_args;
                    }
                    args[count++] = paths[k];
                }
                free(paths);
                free(expanded);
                continue;
            }
        }

        if (count + 1 >= cap)
        {
            cap *= 2;
            char **new_args = realloc(args, (cap + 1) * sizeof(char *));
            if (!new_args)
            {
                free(expanded);
                break;
            }
            args = new_args;
        }
        args[count++] = expanded;
    }

    args[count] = NULL;
    if (out_count)
    {
        *out_count = count;
    }
    return args;
}

void free_glob_args(char **args, int count)
{
    if (!args) return;
    for (int i = 0; i < count; i++)
    {
        free(args[i]);
    }
    free(args);
}
