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
            history_items[history_count++] = strdup(line);
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

    if (history_count >= MAX_HISTORY_ENTRIES)
    {
        free(history_items[0]);
        for (int i = 1; i < MAX_HISTORY_ENTRIES; i++)
        {
            history_items[i - 1] = history_items[i];
        }
        history_items[MAX_HISTORY_ENTRIES - 1] = strdup(cmd);
    }
    else
    {
        history_items[history_count++] = strdup(cmd);
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
