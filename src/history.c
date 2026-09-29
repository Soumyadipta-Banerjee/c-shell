#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *history_items[MAX_HISTORY_ENTRIES];
static int history_count = 0;

static void get_history_path(char *dest, size_t max_len)
{
    const char *home = getenv("HOME");
    if (home)
    {
        snprintf(dest, max_len, "%s/.apex_history", home);
    }
    else
    {
        snprintf(dest, max_len, ".apex_history");
    }
}

void history_init(void)
{
    history_count = 0;
    char path[1024];
    get_history_path(path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f)
    {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f) && history_count < MAX_HISTORY_ENTRIES)
    {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] != '\0')
        {
            char *s = strdup(line);
            if (s)
            {
                history_items[history_count++] = s;
            }
        }
    }
    fclose(f);
}

void history_add(const char *cmd)
{
    if (!cmd)
    {
        return;
    }

    // Skip all-whitespace commands
    const char *p = cmd;
    while (*p && isspace((unsigned char)*p))
    {
        p++;
    }
    if (*p == '\0')
    {
        return;
    }

    // Avoid adjacent duplicate commands
    if (history_count > 0 && strcmp(history_items[history_count - 1], cmd) == 0)
    {
        return;
    }

    char *s = strdup(cmd);
    if (!s)
    {
        return;
    }

    if (history_count >= MAX_HISTORY_ENTRIES)
    {
        free(history_items[0]);
        for (int i = 1; i < MAX_HISTORY_ENTRIES; i++)
        {
            history_items[i - 1] = history_items[i];
        }
        history_items[MAX_HISTORY_ENTRIES - 1] = s;
    }
    else
    {
        history_items[history_count++] = s;
    }
}

void history_save(void)
{
    char path[1024];
    get_history_path(path, sizeof(path));

    FILE *f = fopen(path, "w");
    if (!f)
    {
        return;
    }

    for (int i = 0; i < history_count; i++)
    {
        fprintf(f, "%s\n", history_items[i]);
    }
    fclose(f);
}

void history_cleanup(void)
{
    for (int i = 0; i < history_count; i++)
    {
        free(history_items[i]);
        history_items[i] = NULL;
    }
    history_count = 0;
}

int history_get_count(void)
{
    return history_count;
}

const char *history_get_item(int index)
{
    if (index >= 0 && index < history_count)
    {
        return history_items[index];
    }
    return NULL;
}

char *history_expand(const char *input, int *was_expanded)
{
    if (was_expanded) *was_expanded = 0;
    if (!input) return NULL;

    if (!strchr(input, '!'))
    {
        return strdup(input);
    }

    size_t cap = strlen(input) + 128;
    size_t len = 0;
    char *out = malloc(cap);
    if (!out) return NULL;

    int in_single_quote = 0;
    int in_double_quote = 0;
    int expanded = 0;

    for (size_t i = 0; input[i] != '\0'; )
    {
        char c = input[i];

        if (c == '\\' && !in_single_quote)
        {
            if (input[i + 1] == '!')
            {
                if (len + 1 >= cap) { cap *= 2; char *n = realloc(out, cap); if (!n) { free(out); return NULL; } out = n; }
                out[len++] = '!';
                i += 2;
                continue;
            }
            if (len + 1 >= cap) { cap *= 2; char *n = realloc(out, cap); if (!n) { free(out); return NULL; } out = n; }
            out[len++] = c;
            i++;
            if (input[i] != '\0')
            {
                if (len + 1 >= cap) { cap *= 2; char *n = realloc(out, cap); if (!n) { free(out); return NULL; } out = n; }
                out[len++] = input[i++];
            }
            continue;
        }

        if (c == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            if (len + 1 >= cap) { cap *= 2; char *n = realloc(out, cap); if (!n) { free(out); return NULL; } out = n; }
            out[len++] = c;
            i++;
            continue;
        }

        if (c == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            if (len + 1 >= cap) { cap *= 2; char *n = realloc(out, cap); if (!n) { free(out); return NULL; } out = n; }
            out[len++] = c;
            i++;
            continue;
        }

        if (c == '!' && !in_single_quote)
        {
            char next = input[i + 1];
            if (next == ' ' || next == '\t' || next == '\0' || next == '=' || next == '(')
            {
                if (len + 1 >= cap) { cap *= 2; char *n = realloc(out, cap); if (!n) { free(out); return NULL; } out = n; }
                out[len++] = c;
                i++;
                continue;
            }

            const char *replacement = NULL;
            char last_arg_buf[512] = "";

            if (next == '!')
            {
                if (history_count == 0)
                {
                    fprintf(stderr, "apex-shell: !!: event not found\n");
                    free(out);
                    return NULL;
                }
                replacement = history_items[history_count - 1];
                i += 2;
                expanded = 1;
            }
            else if (next == '$')
            {
                if (history_count == 0)
                {
                    fprintf(stderr, "apex-shell: !$: event not found\n");
                    free(out);
                    return NULL;
                }
                const char *last_cmd = history_items[history_count - 1];
                size_t lcmd_len = strlen(last_cmd);
                while (lcmd_len > 0 && isspace((unsigned char)last_cmd[lcmd_len - 1])) lcmd_len--;
                size_t wend = lcmd_len;
                while (lcmd_len > 0 && !isspace((unsigned char)last_cmd[lcmd_len - 1])) lcmd_len--;
                size_t wstart = lcmd_len;
                size_t wlen = wend - wstart;
                if (wlen >= sizeof(last_arg_buf)) wlen = sizeof(last_arg_buf) - 1;
                memcpy(last_arg_buf, last_cmd + wstart, wlen);
                last_arg_buf[wlen] = '\0';
                replacement = last_arg_buf;
                i += 2;
                expanded = 1;
            }
            else if (isdigit((unsigned char)next))
            {
                size_t start_digits = i + 1;
                size_t end_digits = start_digits;
                while (isdigit((unsigned char)input[end_digits])) end_digits++;
                char numbuf[32];
                size_t nlen = end_digits - start_digits;
                if (nlen >= sizeof(numbuf)) nlen = sizeof(numbuf) - 1;
                memcpy(numbuf, input + start_digits, nlen);
                numbuf[nlen] = '\0';
                int num = atoi(numbuf);
                if (num < 1 || num > history_count)
                {
                    fprintf(stderr, "apex-shell: !%s: event not found\n", numbuf);
                    free(out);
                    return NULL;
                }
                replacement = history_items[num - 1];
                i = end_digits;
                expanded = 1;
            }
            else if (next == '-' && isdigit((unsigned char)input[i + 2]))
            {
                size_t start_digits = i + 2;
                size_t end_digits = start_digits;
                while (isdigit((unsigned char)input[end_digits])) end_digits++;
                char numbuf[32];
                size_t nlen = end_digits - start_digits;
                if (nlen >= sizeof(numbuf)) nlen = sizeof(numbuf) - 1;
                memcpy(numbuf, input + start_digits, nlen);
                numbuf[nlen] = '\0';
                int num = atoi(numbuf);
                if (num < 1 || history_count - num < 0)
                {
                    fprintf(stderr, "apex-shell: !-%s: event not found\n", numbuf);
                    free(out);
                    return NULL;
                }
                replacement = history_items[history_count - num];
                i = end_digits;
                expanded = 1;
            }
            else
            {
                if (len + 1 >= cap) { cap *= 2; char *n = realloc(out, cap); if (!n) { free(out); return NULL; } out = n; }
                out[len++] = c;
                i++;
                continue;
            }

            if (replacement)
            {
                size_t rlen = strlen(replacement);
                while (len + rlen + 1 >= cap)
                {
                    cap = (cap + rlen) * 2;
                    char *n = realloc(out, cap);
                    if (!n) { free(out); return NULL; }
                    out = n;
                }
                memcpy(out + len, replacement, rlen);
                len += rlen;
            }
            continue;
        }

        if (len + 1 >= cap)
        {
            cap *= 2;
            char *n = realloc(out, cap);
            if (!n) { free(out); return NULL; }
            out = n;
        }
        out[len++] = c;
        i++;
    }

    out[len] = '\0';
    if (was_expanded) *was_expanded = expanded;
    return out;
}

int lsh_history(char **args)
{
    if (args[1] != NULL && strcmp(args[1], "-c") == 0)
    {
        history_cleanup();
        char path[1024];
        get_history_path(path, sizeof(path));
        remove(path);
        return 0;
    }

    int start = 0;
    if (args[1] != NULL)
    {
        int limit = atoi(args[1]);
        if (limit > 0 && history_count > limit)
        {
            start = history_count - limit;
        }
    }

    for (int i = start; i < history_count; i++)
    {
        printf("%5d  %s\n", i + 1, history_items[i]);
    }
    fflush(stdout);

    return 0;
}
