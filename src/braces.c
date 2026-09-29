#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "braces.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int is_number(const char *str)
{
    if (!str || !*str) return 0;
    const char *p = str;
    if (*p == '-' || *p == '+') p++;
    if (!*p) return 0;
    while (*p)
    {
        if (!isdigit((unsigned char)*p)) return 0;
        p++;
    }
    return 1;
}

static int find_matching_brace(const char *str, int start_idx, int *out_has_comma, int *out_has_range)
{
    int depth = 0;
    int has_comma = 0;
    int has_range = 0;

    for (int i = start_idx; str[i] != '\0'; i++)
    {
        if (str[i] == '\\' && str[i + 1] != '\0')
        {
            i++;
            continue;
        }

        if (str[i] == '{')
        {
            depth++;
        }
        else if (str[i] == '}')
        {
            depth--;
            if (depth == 0)
            {
                if (out_has_comma) *out_has_comma = has_comma;
                if (out_has_range) *out_has_range = has_range;
                return i;
            }
        }
        else if (depth == 1)
        {
            if (str[i] == ',')
            {
                has_comma = 1;
            }
            else if (str[i] == '.' && str[i + 1] == '.')
            {
                has_range = 1;
            }
        }
    }
    return -1;
}

int has_brace_syntax(const char *str)
{
    if (!str) return 0;

    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == '\\' && str[i + 1] != '\0')
        {
            i++;
            continue;
        }

        if (str[i] == '{')
        {
            int has_comma = 0;
            int has_range = 0;
            int closing = find_matching_brace(str, i, &has_comma, &has_range);
            if (closing > i && (has_comma || has_range))
            {
                return 1;
            }
        }
    }
    return 0;
}

static int add_to_list(char ***list, int *count, int *cap, char *item)
{
    if (!item) return -1;
    if (*count >= *cap)
    {
        int new_cap = (*cap == 0) ? 8 : (*cap * 2);
        char **new_list = realloc(*list, new_cap * sizeof(char *));
        if (!new_list)
        {
            free(item);
            return -1;
        }
        *list = new_list;
        *cap = new_cap;
    }
    (*list)[(*count)++] = item;
    return 0;
}

void free_brace_list(char **list, int count)

{
    if (!list) return;
    for (int i = 0; i < count; i++)
    {
        free(list[i]);
    }
    free(list);
}

static int expand_range(const char *body, char ***out_items, int *out_count)
{
    char start_str[64] = {0};
    char end_str[64] = {0};
    int step = 1;

    const char *dotdot1 = strstr(body, "..");
    if (!dotdot1) return -1;

    size_t start_len = dotdot1 - body;
    if (start_len >= sizeof(start_str)) return -1;
    strncpy(start_str, body, start_len);
    start_str[start_len] = '\0';

    const char *rest = dotdot1 + 2;
    const char *dotdot2 = strstr(rest, "..");
    if (dotdot2)
    {
        size_t end_len = dotdot2 - rest;
        if (end_len >= sizeof(end_str)) return -1;
        strncpy(end_str, rest, end_len);
        end_str[end_len] = '\0';

        const char *step_str = dotdot2 + 2;
        if (is_number(step_str))
        {
            step = atoi(step_str);
            if (step <= 0) step = 1;
        }
    }
    else
    {
        strncpy(end_str, rest, sizeof(end_str) - 1);
    }

    int count = 0;
    int cap = 16;
    char **items = malloc(cap * sizeof(char *));
    if (!items) return -1;

    if (is_number(start_str) && is_number(end_str))
    {
        long long start = atoll(start_str);
        long long end = atoll(end_str);

        if (start <= end)
        {
            for (long long v = start; v <= end; v += step)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%lld", v);
                add_to_list(&items, &count, &cap, strdup(buf));
            }
        }
        else
        {
            for (long long v = start; v >= end; v -= step)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%lld", v);
                add_to_list(&items, &count, &cap, strdup(buf));
            }
        }
        *out_items = items;
        *out_count = count;
        return 0;
    }
    else if (strlen(start_str) == 1 && strlen(end_str) == 1 &&
             isalpha((unsigned char)start_str[0]) && isalpha((unsigned char)end_str[0]))
    {
        char c_start = start_str[0];
        char c_end = end_str[0];

        if (c_start <= c_end)
        {
            for (char c = c_start; c <= c_end; c += step)
            {
                char buf[2] = { c, '\0' };
                add_to_list(&items, &count, &cap, strdup(buf));
            }
        }
        else
        {
            for (char c = c_start; c >= c_end; c -= step)
            {
                char buf[2] = { c, '\0' };
                add_to_list(&items, &count, &cap, strdup(buf));
            }
        }
        *out_items = items;
        *out_count = count;
        return 0;
    }

    free_brace_list(items, count);
    return -1;
}

static int split_comma_items(const char *body, char ***out_items, int *out_count)
{
    int count = 0;
    int cap = 8;
    char **items = malloc(cap * sizeof(char *));
    if (!items) return -1;

    int depth = 0;
    const char *start = body;
    for (const char *p = body;; p++)
    {
        if (*p == '\\' && *(p + 1) != '\0')
        {
            p++;
            continue;
        }

        if (*p == '{')
        {
            depth++;
        }
        else if (*p == '}')
        {
            if (depth > 0) depth--;
        }
        else if ((*p == ',' && depth == 0) || *p == '\0')
        {
            size_t seg_len = p - start;
            char *seg = malloc(seg_len + 1);
            if (seg)
            {
                strncpy(seg, start, seg_len);
                seg[seg_len] = '\0';
                add_to_list(&items, &count, &cap, seg);
            }
            if (*p == '\0') break;
            start = p + 1;
        }
    }

    *out_items = items;
    *out_count = count;
    return 0;
}

int expand_braces(const char *input, char ***out_list, int *out_count)
{
    if (!input)
    {
        if (out_count) *out_count = 0;
        if (out_list) *out_list = NULL;
        return -1;
    }

    // Look for first expandable brace
    int brace_start = -1;
    int brace_end = -1;
    int has_comma = 0;
    int has_range = 0;

    for (int i = 0; input[i] != '\0'; i++)
    {
        if (input[i] == '\\' && input[i + 1] != '\0')
        {
            i++;
            continue;
        }
        if (input[i] == '{')
        {
            int hc = 0, hr = 0;
            int end = find_matching_brace(input, i, &hc, &hr);
            if (end > i && (hc || hr))
            {
                brace_start = i;
                brace_end = end;
                has_comma = hc;
                has_range = hr;
                break;
            }
        }
    }

    // If no valid brace expansion found, return input as single item
    if (brace_start < 0)
    {
        char **res = malloc(sizeof(char *));
        if (!res) return -1;
        res[0] = strdup(input);
        *out_list = res;
        *out_count = 1;
        return 0;
    }

    // Extract prefix
    size_t prefix_len = brace_start;
    char *prefix = malloc(prefix_len + 1);
    if (!prefix) return -1;
    strncpy(prefix, input, prefix_len);
    prefix[prefix_len] = '\0';

    // Extract suffix
    const char *suffix = input + brace_end + 1;

    // Extract body
    size_t body_len = brace_end - brace_start - 1;
    char *body = malloc(body_len + 1);
    if (!body)
    {
        free(prefix);
        return -1;
    }
    strncpy(body, input + brace_start + 1, body_len);
    body[body_len] = '\0';

    char **items = NULL;
    int item_count = 0;

    if (has_range && expand_range(body, &items, &item_count) == 0)
    {
        // Handled as range
    }
    else if (has_comma && split_comma_items(body, &items, &item_count) == 0)
    {
        // Handled as comma list
    }

    free(body);

    if (!items || item_count == 0)
    {
        free(prefix);
        if (items) free_brace_list(items, item_count);
        char **res = malloc(sizeof(char *));
        if (!res) return -1;
        res[0] = strdup(input);
        *out_list = res;
        *out_count = 1;
        return 0;
    }

    int res_count = 0;
    int res_cap = 16;
    char **res_list = malloc(res_cap * sizeof(char *));
    if (!res_list)
    {
        free(prefix);
        free_brace_list(items, item_count);
        return -1;
    }

    for (int i = 0; i < item_count; i++)
    {
        size_t combined_len = prefix_len + strlen(items[i]) + strlen(suffix) + 1;
        char *combined = malloc(combined_len);
        if (!combined) continue;

        snprintf(combined, combined_len, "%s%s%s", prefix, items[i], suffix);

        // Recursively expand any remaining braces
        char **sub_list = NULL;
        int sub_count = 0;
        if (expand_braces(combined, &sub_list, &sub_count) == 0 && sub_list)
        {
            for (int s = 0; s < sub_count; s++)
            {
                add_to_list(&res_list, &res_count, &res_cap, sub_list[s]);
            }
            free(sub_list); // Free container only; items transferred to res_list
        }
        else
        {
            add_to_list(&res_list, &res_count, &res_cap, strdup(combined));
        }

        free(combined);
    }

    free(prefix);
    free_brace_list(items, item_count);

    *out_list = res_list;
    *out_count = res_count;
    return 0;
}
