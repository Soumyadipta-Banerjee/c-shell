#ifndef APEX_GLOBBER_H
#define APEX_GLOBBER_H

#include "parser.h"

/* Returns 1 if string contains unquoted wildcard metacharacters (*, ?, [), 0 otherwise */
int has_glob_meta(const char *str);

/* Expands a wildcard path pattern into an array of matching paths.
   Returns 0 on success, non-zero on error. */
int expand_path_pattern(const char *pattern, char ***out_paths, int *out_count);

/* Frees a list of paths returned by expand_path_pattern */
void free_path_list(char **paths, int count);

/* Expands a range of ShellTokens into an argument list, performing variable
   expansion and wildcard globbing on unquoted tokens. Returns NULL-terminated array. */
char **expand_tokens_with_glob(ShellToken **tokens, int start, int len, int *out_count);

/* Frees an argument list allocated by expand_tokens_with_glob */
void free_glob_args(char **args, int count);

#endif /* APEX_GLOBBER_H */
