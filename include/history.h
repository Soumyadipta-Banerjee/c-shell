#ifndef APEX_HISTORY_H
#define APEX_HISTORY_H

#define MAX_HISTORY_ENTRIES 1000

/* Initialize history and load from ~/.apex_history */
void history_init(void);

/* Add a command string to history */
void history_add(const char *cmd);

/* Save history to ~/.apex_history */
void history_save(void);

/* Clean up history memory */
void history_cleanup(void);

/* Get number of history entries */
int history_get_count(void);

/* Get history item by 0-based index */
const char *history_get_item(int index);

/* Expand history references (!!, !$, !n, !-n) in input line.
 * Returns newly allocated string or NULL on error / missing event.
 * Sets *was_expanded to 1 if expansion occurred, 0 otherwise.
 */
char *history_expand(const char *input, int *was_expanded);

/* Built-in history command */
int lsh_history(char **args);

#endif /* APEX_HISTORY_H */
