#ifndef APEX_ALIAS_H
#define APEX_ALIAS_H

#define MAX_ALIASES 256

typedef struct {
    char *name;
    char *value;
} AliasEntry;

/* Initialize alias subsystem */
void alias_init(void);

/* Clean up alias subsystem memory */
void alias_cleanup(void);

/* Set an alias (name and value) */
int alias_set(const char *name, const char *value);

/* Remove an alias */
int alias_unset(const char *name);

/* Get alias value for a given name, returns NULL if not found */
const char *alias_get(const char *name);

/* Built-in 'alias' command */
int lsh_alias(char **args);

/* Built-in 'unalias' command */
int lsh_unalias(char **args);

#endif /* APEX_ALIAS_H */
