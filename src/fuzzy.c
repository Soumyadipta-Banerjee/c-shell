#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "fuzzy.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

static const char *popular_commands[] = {
    "git", "gcc", "grep", "make", "cat", "curl", "find",
    "diff", "echo", "chmod", "kill", "ps", "tar", "ssh",
    "clear", "top", "nano", "vim", "man", "sudo", "ls"
};

static int is_popular_command(const char *name)
{
    size_t count = sizeof(popular_commands) / sizeof(popular_commands[0]);
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(name, popular_commands[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int levenshtein_distance(const char *s1, const char *s2)
{
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
    if (len1 > 128 || len2 > 128) return 999;

    int d[130][130];

    for (int i = 0; i <= len1; i++) d[i][0] = i;
    for (int j = 0; j <= len2; j++) d[0][j] = j;

    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            int del_cost = d[i - 1][j] + 1;
            int ins_cost = d[i][j - 1] + 1;
            int sub_cost = d[i - 1][j - 1] + cost;

            int min_cost = del_cost;
            if (ins_cost < min_cost) min_cost = ins_cost;
            if (sub_cost < min_cost) min_cost = sub_cost;

            // Damerau-Levenshtein transposition
            if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] && s1[i - 2] == s2[j - 1])
            {
                int trans_cost = d[i - 2][j - 2] + 1;
                if (trans_cost < min_cost)
                {
                    min_cost = trans_cost;
                }
            }

            d[i][j] = min_cost;
        }
    }

    return d[len1][len2];
}

char *find_closest_command(const char *target, int max_dist)
{
    int tlen = (int)strlen(target);
    if (tlen == 0)
    {
        return NULL;
    }

    int best_dist = max_dist + 1;
    char *best_match = NULL;
    int best_is_builtin = 0;
    int best_is_popular = 0;

    // 1. Scan built-in commands first
    for (int i = 0; i < lsh_num_builtins(); i++)
    {
        const char *bname = builtin_str[i];
        int blen = (int)strlen(bname);
        if (abs(blen - tlen) <= max_dist)
        {
            int dist = levenshtein_distance(target, bname);
            if (dist < best_dist)
            {
                best_dist = dist;
                free(best_match);
                best_match = strdup(bname);
                best_is_builtin = 1;
                best_is_popular = 1;
            }
        }
    }

    // 2. Scan directories in PATH
    const char *path_env = getenv("PATH");
    if (!path_env)
    {
        path_env = "/usr/bin:/bin:/usr/local/bin";
    }

    char *path_copy = strdup(path_env);
    if (path_copy)
    {
        char *saveptr = NULL;
        char *dir_path = strtok_r(path_copy, ":", &saveptr);
        while (dir_path != NULL)
        {
            DIR *d = opendir(dir_path);
            if (d)
            {
                struct dirent *entry;
                while ((entry = readdir(d)) != NULL)
                {
                    if (entry->d_name[0] == '.')
                    {
                        continue;
                    }

                    int nlen = (int)strlen(entry->d_name);
                    if (abs(nlen - tlen) > max_dist)
                    {
                        continue;
                    }

                    char full_path[1024];
                    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
                    if (access(full_path, X_OK) == 0)
                    {
                        int dist = levenshtein_distance(target, entry->d_name);
                        int is_pop = is_popular_command(entry->d_name);

                        int should_replace = 0;
                        if (dist < best_dist)
                        {
                            should_replace = 1;
                        }
                        else if (dist == best_dist)
                        {
                            // If tied with a built-in, keep built-in
                            if (!best_is_builtin)
                            {
                                // Prefer popular command
                                if (is_pop && !best_is_popular)
                                {
                                    should_replace = 1;
                                }
                                else if (is_pop == best_is_popular && entry->d_name[0] == target[0] && best_match && best_match[0] != target[0])
                                {
                                    should_replace = 1;
                                }
                            }
                        }

                        if (should_replace)
                        {
                            best_dist = dist;
                            free(best_match);
                            best_match = strdup(entry->d_name);
                            best_is_builtin = 0;
                            best_is_popular = is_pop;
                        }
                    }
                }
                closedir(d);
            }
            dir_path = strtok_r(NULL, ":", &saveptr);
        }
        free(path_copy);
    }

    if (best_dist <= max_dist)
    {
        return best_match;
    }

    free(best_match);
    return NULL;
}

void print_command_not_found(const char *cmd)
{
    fprintf(stderr, "apex-shell: '%s' command not found\n", cmd);
    char *suggestion = find_closest_command(cmd, 2);
    if (suggestion)
    {
        fprintf(stderr, "Did you mean: '%s'?\n", suggestion);
        free(suggestion);
    }
}
