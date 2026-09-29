#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "linereader.h"
#include "parser.h"
#include "history.h"
#include "highlight.h"
#include "completion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>



static const char *get_ghost_suggestion(const char *buf, size_t len)
{
    if (len == 0) return NULL;
    int count = history_get_count();
    for (int i = count - 1; i >= 0; i--)
    {
        const char *item = history_get_item(i);
        if (item && strncmp(item, buf, len) == 0 && strlen(item) > len)
        {
            return item + len;
        }
    }
    return NULL;
}

static void refresh_line(const char *buf, size_t len, size_t pos)
{
    printf("\r");
    lsh_print_prompt();
    if (len > 0)
    {
        print_syntax_highlighted(buf, len);
    }

    const char *ghost = NULL;
    if (pos == len)
    {
        ghost = get_ghost_suggestion(buf, len);
    }

    if (ghost)
    {
        printf("\033[90m%s\033[0m", ghost); // Dim / gray ghost text
        printf("\033[K");
        printf("\033[%dD", (int)strlen(ghost)); // Keep cursor at pos
    }
    else
    {
        printf("\033[K");
        if (pos < len)
        {
            printf("\033[%dD", (int)(len - pos));
        }
    }
    fflush(stdout);
}



char *lsh_read_interactive_line(void)
{
    if (!isatty(STDIN_FILENO))
    {
        return lsh_read_line();
    }

    struct termios orig_termios, raw;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    {
        return lsh_read_line();
    }

    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
        return lsh_read_line();
    }

    char buf[4096];
    size_t len = 0;
    size_t pos = 0;
    buf[0] = '\0';

    int history_index = history_get_count();
    char saved_line[4096] = "";

    while (1)
    {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0)
        {
            break;
        }

        // Enter: submit line
        if (c == '\r' || c == '\n')
        {
            printf("\n");
            break;
        }

        // Ctrl+A: beginning of line (Home)
        if (c == 1)
        {
            pos = 0;
            refresh_line(buf, len, pos);
            continue;
        }

        // Ctrl+E: end of line (and accept ghost suggestion if at end)
        if (c == 5)
        {
            if (pos == len)
            {
                const char *ghost = get_ghost_suggestion(buf, len);
                if (ghost)
                {
                    size_t glen = strlen(ghost);
                    if (len + glen < sizeof(buf))
                    {
                        memcpy(buf + len, ghost, glen);
                        len += glen;
                        buf[len] = '\0';
                    }
                }
            }
            pos = len;
            refresh_line(buf, len, pos);
            continue;
        }

        // Ctrl+F: accept ghost suggestion
        if (c == 6)
        {
            if (pos == len)
            {
                const char *ghost = get_ghost_suggestion(buf, len);
                if (ghost)
                {
                    size_t glen = strlen(ghost);
                    if (len + glen < sizeof(buf))
                    {
                        memcpy(buf + len, ghost, glen);
                        len += glen;
                        pos = len;
                        buf[len] = '\0';
                        refresh_line(buf, len, pos);
                    }
                }
            }
            continue;
        }

        // Ctrl+C: cancel current line
        if (c == 3)
        {
            printf("^C\n");
            buf[0] = '\0';
            len = 0;
            pos = 0;
            break;
        }

        // Ctrl+D: EOF
        if (c == 4)
        {
            if (len == 0)
            {
                printf("\n");
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
                return NULL;
            }
            continue;
        }

        // Ctrl+L: clear screen
        if (c == 12)
        {
            printf("\033[H\033[2J");
            refresh_line(buf, len, pos);
            continue;
        }

        // Ctrl+R: Reverse History Search
        if (c == 18)
        {
            char query[256] = "";
            size_t qlen = 0;
            int total_hist = history_get_count();
            int match_idx = -1;

            for (int i = total_hist - 1; i >= 0; i--)
            {
                const char *item = history_get_item(i);
                if (item && item[0] != '\0')
                {
                    match_idx = i;
                    break;
                }
            }

            char last_sc = 0;
            while (1)
            {
                const char *matched_str = (match_idx >= 0) ? history_get_item(match_idx) : "";
                if (!matched_str) matched_str = "";
                printf("\r(reverse-i-search)`%s': %s\033[K", query, matched_str);
                fflush(stdout);

                char sc;
                if (read(STDIN_FILENO, &sc, 1) <= 0) break;
                last_sc = sc;

                if (sc == 18) // Ctrl+R: cycle backwards
                {
                    if (match_idx > 0)
                    {
                        int next_idx = match_idx - 1;
                        while (next_idx >= 0)
                        {
                            const char *item = history_get_item(next_idx);
                            if (item && (qlen == 0 || strstr(item, query) != NULL))
                            {
                                match_idx = next_idx;
                                break;
                            }
                            next_idx--;
                        }
                    }
                }
                else if (sc == 127 || sc == 8) // Backspace
                {
                    if (qlen > 0)
                    {
                        query[--qlen] = '\0';
                        match_idx = -1;
                        for (int i = total_hist - 1; i >= 0; i--)
                        {
                            const char *item = history_get_item(i);
                            if (item && (qlen == 0 || strstr(item, query) != NULL))
                            {
                                match_idx = i;
                                break;
                            }
                        }
                    }
                }
                else if (sc == '\r' || sc == '\n') // Enter: accept & execute
                {
                    if (match_idx >= 0)
                    {
                        const char *item = history_get_item(match_idx);
                        if (item)
                        {
                            strncpy(buf, item, sizeof(buf) - 1);
                            buf[sizeof(buf) - 1] = '\0';
                            len = strlen(buf);
                            pos = len;
                        }
                    }
                    printf("\n");
                    break;
                }
                else if (sc == 27 || sc == 7) // Esc or Ctrl+G: abort search
                {
                    strncpy(buf, saved_line, sizeof(buf) - 1);
                    buf[sizeof(buf) - 1] = '\0';
                    len = strlen(buf);
                    pos = len;
                    refresh_line(buf, len, pos);
                    break;
                }
                else if (isprint((unsigned char)sc)) // Character typed
                {
                    if (qlen < sizeof(query) - 1)
                    {
                        query[qlen++] = sc;
                        query[qlen] = '\0';
                        match_idx = -1;
                        for (int i = total_hist - 1; i >= 0; i--)
                        {
                            const char *item = history_get_item(i);
                            if (item && strstr(item, query) != NULL)
                            {
                                match_idx = i;
                                break;
                            }
                        }
                    }
                }
                else // Other key: commit match and exit search
                {
                    if (match_idx >= 0)
                    {
                        const char *item = history_get_item(match_idx);
                        if (item)
                        {
                            strncpy(buf, item, sizeof(buf) - 1);
                            buf[sizeof(buf) - 1] = '\0';
                            len = strlen(buf);
                            pos = len;
                        }
                    }
                    refresh_line(buf, len, pos);
                    break;
                }
            }
            if (last_sc == '\r' || last_sc == '\n')
            {
                break;
            }
            continue;
        }

        // Tab: autocompletion
        if (c == '\t')
        {
            complete_word(buf, &len, &pos, sizeof(buf));
            refresh_line(buf, len, pos);
            continue;
        }

        // Backspace
        if (c == 127 || c == 8)
        {
            if (pos > 0)
            {
                memmove(buf + pos - 1, buf + pos, len - pos);
                len--;
                pos--;
                buf[len] = '\0';
                refresh_line(buf, len, pos);
            }
            continue;
        }

        // Escape sequence (arrows, home, end, del)
        if (c == '\033')
        {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;

            if (seq[0] == '[')
            {
                if (seq[1] == 'A') // Up Arrow: history previous
                {
                    if (history_index > 0)
                    {
                        if (history_index == history_get_count())
                        {
                            snprintf(saved_line, sizeof(saved_line), "%s", buf);
                        }
                        history_index--;
                        const char *item = history_get_item(history_index);
                        if (item)
                        {
                            snprintf(buf, sizeof(buf), "%s", item);
                            len = strlen(buf);
                            pos = len;
                            refresh_line(buf, len, pos);
                        }
                    }
                }
                else if (seq[1] == 'B') // Down Arrow: history next
                {
                    if (history_index < history_get_count())
                    {
                        history_index++;
                        if (history_index == history_get_count())
                        {
                            snprintf(buf, sizeof(buf), "%s", saved_line);
                        }
                        else
                        {
                            const char *item = history_get_item(history_index);
                            if (item)
                            {
                                snprintf(buf, sizeof(buf), "%s", item);
                            }
                        }
                        len = strlen(buf);
                        pos = len;
                        refresh_line(buf, len, pos);
                    }
                }
                else if (seq[1] == 'C') // Right Arrow
                {
                    if (pos < len)
                    {
                        pos++;
                        refresh_line(buf, len, pos);
                    }
                    else if (pos == len)
                    {
                        const char *ghost = get_ghost_suggestion(buf, len);
                        if (ghost)
                        {
                            size_t glen = strlen(ghost);
                            if (len + glen < sizeof(buf))
                            {
                                memcpy(buf + len, ghost, glen);
                                len += glen;
                                pos = len;
                                buf[len] = '\0';
                                refresh_line(buf, len, pos);
                            }
                        }
                    }
                }
                else if (seq[1] == 'D') // Left Arrow
                {
                    if (pos > 0)
                    {
                        pos--;
                        refresh_line(buf, len, pos);
                    }
                }
                else if (seq[1] == 'H') // Home
                {
                    pos = 0;
                    refresh_line(buf, len, pos);
                }
                else if (seq[1] == 'F') // End
                {
                    if (pos == len)
                    {
                        const char *ghost = get_ghost_suggestion(buf, len);
                        if (ghost)
                        {
                            size_t glen = strlen(ghost);
                            if (len + glen < sizeof(buf))
                            {
                                memcpy(buf + len, ghost, glen);
                                len += glen;
                                buf[len] = '\0';
                            }
                        }
                    }
                    pos = len;
                    refresh_line(buf, len, pos);
                }
                else if (seq[1] == '3') // Delete key
                {
                    char extra;
                    if (read(STDIN_FILENO, &extra, 1) > 0 && extra == '~')
                    {
                        if (pos < len)
                        {
                            memmove(buf + pos, buf + pos + 1, len - pos - 1);
                            len--;
                            buf[len] = '\0';
                            refresh_line(buf, len, pos);
                        }
                    }
                }
            }
            continue;
        }

        // Regular printable character
        if (isprint((unsigned char)c))
        {
            if (len + 1 < sizeof(buf))
            {
                memmove(buf + pos + 1, buf + pos, len - pos);
                buf[pos] = c;
                len++;
                pos++;
                buf[len] = '\0';
                refresh_line(buf, len, pos);
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    return strdup(buf);
}
