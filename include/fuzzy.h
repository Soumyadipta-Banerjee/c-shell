#ifndef APEX_FUZZY_H
#define APEX_FUZZY_H

/* Computes the Levenshtein distance between two strings */
int levenshtein_distance(const char *s1, const char *s2);

/* Finds the closest matching command among built-ins and PATH binaries.
   Returns heap-allocated string if match within max_dist found, otherwise NULL. */
char *find_closest_command(const char *target, int max_dist);

/* Prints command not found error with intelligent 'Did you mean' suggestion */
void print_command_not_found(const char *cmd);

#endif /* APEX_FUZZY_H */
