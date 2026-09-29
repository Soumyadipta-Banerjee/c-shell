#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "completion.h"
#include "builtins.h"
#include "alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

extern char **environ;

void complete_word(char *buf, size_t *len, size_t *pos, size_t buf_max)
{
    size_t start = *pos;
    while (start > 0 && buf[start - 1] != ' ' && buf[start - 1] != '\t' &&
           buf[start - 1] != '|' && buf[start - 1] != '&' && buf[start - 1] != ';')
    {
        start--;
    }

    size_t wlen = *pos - start;
    char word[256];
    if (wlen >= sizeof(word))
    {
        wlen = sizeof(word) - 1;
    }
    memcpy(word, buf + start, wlen);
    word[wlen] = '\0';

    int is_command = 1;
    for (size_t i = 0; i < start; i++)
    {
        if (buf[i] != ' ' && buf[i] != '\t')
        {
            is_command = 0;
            break;
        }
    }

    char *matches[256];
    int match_count = 0;

    // 1. Built-in and Alias command completion
    if (is_command)
    {
        for (int i = 0; i < lsh_num_builtins() && match_count < 256; i++)
        {
            if (strncmp(builtin_str[i], word, wlen) == 0)
            {
                matches[match_count++] = strdup(builtin_str[i]);
            }
        }
        for (int i = 0; i < alias_get_count() && match_count < 256; i++)
        {
            const char *aname = alias_get_name(i);
            if (aname && strncmp(aname, word, wlen) == 0)
            {
                matches[match_count++] = strdup(aname);
            }
        }
        if (strncmp("time", word, wlen) == 0 && match_count < 256)
        {
            matches[match_count++] = strdup("time");
        }
    }

    // 2. Environment variable completion
    if (word[0] == '$' && match_count < 256)
    {
        const char *var_prefix = word + 1;
        size_t vlen = strlen(var_prefix);
        if (environ)
        {
            for (char **env = environ; *env != NULL && match_count < 256; env++)
            {
                char *eq = strchr(*env, '=');
                if (!eq) continue;
                size_t klen = (size_t)(eq - *env);
                if (klen >= vlen && strncmp(*env, var_prefix, vlen) == 0)
                {
                    char comp[256];
                    if (klen + 2 < sizeof(comp))
                    {
                        comp[0] = '$';
                        memcpy(comp + 1, *env, klen);
                        comp[klen + 1] = '\0';
                        matches[match_count++] = strdup(comp);
                    }
                }
            }
        }
    }

    // 3. File / Directory completion (if not completing a variable)
    if (word[0] != '$')
    {
    char dir_path[256] = ".";
    const char *file_prefix = word;
    char *last_slash = strrchr(word, '/');
    if (last_slash)
    {
        size_t dlen = last_slash - word;
        if (dlen == 0)
        {
            strcpy(dir_path, "/");
        }
        else
        {
            strncpy(dir_path, word, dlen);
            dir_path[dlen] = '\0';
        }
        file_prefix = last_slash + 1;
    }

    DIR *dir = opendir(dir_path);
    if (dir)
    {
        struct dirent *ent;
        size_t flen = strlen(file_prefix);
        while ((ent = readdir(dir)) != NULL && match_count < 256)
        {
            if (ent->d_name[0] == '.' && flen == 0)
            {
                continue;
            }
            if (strncmp(ent->d_name, file_prefix, flen) == 0)
            {
                char full[512];
                snprintf(full, sizeof(full), "%s/%s", dir_path, ent->d_name);
                struct stat st;
                int is_dir = (stat(full, &st) == 0 && S_ISDIR(st.st_mode));

                char comp[512];
                if (last_slash)
                {
                    char dprefix[256];
                    strncpy(dprefix, word, last_slash - word + 1);
                    dprefix[last_slash - word + 1] = '\0';
                    snprintf(comp, sizeof(comp), "%s%s%s", dprefix, ent->d_name, is_dir ? "/" : "");
                }
                else
                {
                    snprintf(comp, sizeof(comp), "%s%s", ent->d_name, is_dir ? "/" : "");
                }
                matches[match_count++] = strdup(comp);
            }
        }
        closedir(dir);
    }
    }

    if (match_count == 1)
    {
        const char *m = matches[0];
        size_t mlen = strlen(m);
        int add_space = (m[mlen - 1] != '/');
        size_t new_part_len = mlen + (add_space ? 1 : 0);

        if (start + new_part_len + (*len - *pos) < buf_max)
        {
            memmove(buf + start + new_part_len, buf + *pos, *len - *pos);
            memcpy(buf + start, m, mlen);
            if (add_space)
            {
                buf[start + mlen] = ' ';
            }
            *len = *len - (*pos - start) + new_part_len;
            *pos = start + new_part_len;
            buf[*len] = '\0';
        }
    }
    else if (match_count > 1)
    {
        // Find longest common prefix of matches
        size_t common_len = strlen(matches[0]);
        for (int i = 1; i < match_count; i++)
        {
            size_t j = 0;
            while (j < common_len && matches[0][j] == matches[i][j])
            {
                j++;
            }
            common_len = j;
        }

        if (common_len > wlen)
        {
            if (start + common_len + (*len - *pos) < buf_max)
            {
                memmove(buf + start + common_len, buf + *pos, *len - *pos);
                memcpy(buf + start, matches[0], common_len);
                *len = *len - (*pos - start) + common_len;
                *pos = start + common_len;
                buf[*len] = '\0';
            }
        }
        else
        {
            printf("\n");
            for (int i = 0; i < match_count; i++)
            {
                printf("%s  ", matches[i]);
            }
            printf("\n");
        }
    }

    for (int i = 0; i < match_count; i++)
    {
        free(matches[i]);
    }
}
