#ifndef APEX_SAFETY_H
#define APEX_SAFETY_H

/* Initialize safety shield */
void safety_init(void);

/* Check if command is dangerous. Returns 0 if safe/confirmed, 1 if blocked. */
int safety_check_command(char **args);

/* Built-in 'safemode' command to toggle or query shield state */
int lsh_safemode(char **args);

#endif /* APEX_SAFETY_H */
