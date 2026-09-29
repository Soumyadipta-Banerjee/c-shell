#ifndef BRACES_H
#define BRACES_H

#include <stddef.h>

/**
 * Checks if a string contains unquoted brace expansion syntax.
 * Returns 1 if candidate braces are found, 0 otherwise.
 */
int has_brace_syntax(const char *str);

/**
 * Performs brace expansion on input token.
 * Expands comma lists ({a,b,c}), numeric ranges ({1..5}, {5..1}, {1..10..2}),
 * and character ranges ({a..e}, {e..a}). Handles cartesian products ({A,B}{1,2}).
 *
 * Populates out_list with heap-allocated string pointers.
 * out_count is set to the number of resulting strings.
 *
 * Returns 0 on success, non-zero on error.
 */
int expand_braces(const char *input, char ***out_list, int *out_count);

/**
 * Frees array of strings allocated by expand_braces.
 */
void free_brace_list(char **list, int count);

#endif // BRACES_H
