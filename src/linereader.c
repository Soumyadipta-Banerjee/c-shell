#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "linereader.h"
#include "parser.h"
#include "history.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

static void refresh_line(const char *buf, size_t len, size_t pos)
{
    printf("\r");
    lsh_print_prompt();
    if (len > 0)
    {
        fwrite(buf, 1, len, stdout);
    }
    printf("\033[K");
    if (pos < len)
    {
        printf("\033[%dD", (int)(len - pos));
    }
    fflush(stdout);
}

static void complete_word(char *buf, size_t *len, size_t *pos, size_t buf_max)
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

    // 1. Built-in command completion
    if (is_command)
    {
        for (int i = 0; i < lsh_num_builtins() && match_count < 256; i++)
        {
            if (strncmp(builtin_str[i], word, wlen) == 0)
            {
                matches[match_count++] = strdup(builtin_str[i]);
            }
        }
        if (strncmp("time", word, wlen) == 0 && match_count < 256)
        {
            matches[match_count++] = strdup("time");
        }
    }

    // 2. File / Directory completion
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
