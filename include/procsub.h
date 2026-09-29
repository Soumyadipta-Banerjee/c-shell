#ifndef PROCSUB_H
#define PROCSUB_H

/**
 * Checks if a command line string contains unquoted process substitution
 * syntax: <(...) or >(...)
 */
int has_process_substitution(const char *line);

/**
 * Parses and resolves process substitutions <(...) and >(...) in the command line.
 * Spawns subshell children connected to anonymous pipes and replaces
 * the syntax with /dev/fd/<fd>.
 *
 * Returns a newly heap-allocated string with substituted /dev/fd paths.
 * Caller is responsible for free()-ing the returned string.
 */
char *resolve_process_substitutions(const char *line);

/**
 * Reaps any background process substitution subshells spawned during the
 * execution of the current line and closes open pipe file descriptors.
 */
void reap_process_substitutions(void);

#endif // PROCSUB_H
