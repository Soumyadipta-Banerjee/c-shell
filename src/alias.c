#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AliasEntry aliases[MAX_ALIASES];
static int alias_count = 0;

void alias_init(void)
{
    alias_count = 0;
    for (int i = 0; i < MAX_ALIASES; i++)
    {
        aliases[i].name = NULL;
        aliases[i].value = NULL;
    }
}

void alias_cleanup(void)
{
    for (int i = 0; i < alias_count; i++)
    {
        free(aliases[i].name);
        free(aliases[i].value);
        aliases[i].name = NULL;
        aliases[i].value = NULL;
    }
    alias_count = 0;
}

const char *alias_get(const char *name)
{
    if (!name)
    {
        return NULL;
    }
    for (int i = 0; i < alias_count; i++)
    {
        if (strcmp(aliases[i].name, name) == 0)
        {
            return aliases[i].value;
        }
    }
    return NULL;
}

int alias_get_count(void)
{
    return alias_count;
}

const char *alias_get_name(int index)
{
    if (index >= 0 && index < alias_count)
    {
        return aliases[index].name;
    }
    return NULL;
}

int alias_set(const char *name, const char *value)
{
    if (!name || !value)
    {
        return -1;
    }

    // Check if alias already exists, update value
    for (int i = 0; i < alias_count; i++)
    {
        if (strcmp(aliases[i].name, name) == 0)
        {
            free(aliases[i].value);
            aliases[i].value = strdup(value);
            return 0;
        }
    }

    if (alias_count >= MAX_ALIASES)
    {
        fprintf(stderr, "apex-shell: alias table full\n");
        return -1;
    }

    aliases[alias_count].name = strdup(name);
    aliases[alias_count].value = strdup(value);
    alias_count++;
    return 0;
}

int alias_unset(const char *name)
{
    if (!name)
    {
        return -1;
    }
    for (int i = 0; i < alias_count; i++)
    {
        if (strcmp(aliases[i].name, name) == 0)
        {
            free(aliases[i].name);
            free(aliases[i].value);
            for (int j = i + 1; j < alias_count; j++)
            {
                aliases[j - 1] = aliases[j];
            }
            alias_count--;
            aliases[alias_count].name = NULL;
            aliases[alias_count].value = NULL;
            return 0;
        }
    }
    return -1;
}

int lsh_alias(char **args)
{
    if (args[1] == NULL)
    {
        for (int i = 0; i < alias_count; i++)
        {
            printf("alias %s='%s'\n", aliases[i].name, aliases[i].value);
        }
        return 0;
    }

    int ret = 0;
    for (int i = 1; args[i] != NULL; i++)
    {
        char *eq = strchr(args[i], '=');
        if (eq == NULL)
        {
            const char *val = alias_get(args[i]);
            if (val)
            {
                printf("alias %s='%s'\n", args[i], val);
            }
            else
            {
                fprintf(stderr, "apex-shell: alias: %s: not found\n", args[i]);
                ret = 1;
            }
        }
        else
        {
            *eq = '\0';
            char *name = args[i];
            char *val = eq + 1;

            // Strip enclosing quotes if present (e.g. 'val' or "val")
            size_t vlen = strlen(val);
            if (vlen >= 2 && ((val[0] == '\'' && val[vlen - 1] == '\'') ||
                             (val[0] == '"' && val[vlen - 1] == '"')))
            {
                val[vlen - 1] = '\0';
                val++;
            }

            alias_set(name, val);
        }
    }
    return ret;
}

int lsh_unalias(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "apex-shell: unalias: usage: unalias [-a] name [name ...]\n");
        return 1;
    }

    if (strcmp(args[1], "-a") == 0)
    {
        alias_cleanup();
        return 0;
    }

    int ret = 0;
    for (int i = 1; args[i] != NULL; i++)
    {
        if (alias_unset(args[i]) != 0)
        {
            fprintf(stderr, "apex-shell: unalias: %s: not found\n", args[i]);
            ret = 1;
        }
    }
    return ret;
}
