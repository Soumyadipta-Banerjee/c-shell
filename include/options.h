#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    int errexit; // -e / +e: exit on non-zero command status
    int xtrace;  // -x / +x: trace execution to stderr
    int nounset; // -u / +u: error on unset variable expansion
} ShellOptions;

extern ShellOptions g_shell_opts;

/**
 * Built-in 'set' command implementation.
 * Supports: set -e, set +e, set -x, set +x, set -u, set +u,
 * and bare 'set' to display active options and environment.
 */
int lsh_set(char **args);

#endif // OPTIONS_H
