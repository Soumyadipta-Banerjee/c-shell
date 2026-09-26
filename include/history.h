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

/* Built-in history command */
int lsh_history(char **args);

#endif /* APEX_HISTORY_H */
