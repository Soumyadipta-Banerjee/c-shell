#ifndef APEX_PROMPT_H
#define APEX_PROMPT_H

#include <stddef.h>

/* Displays dynamic user@host:path (git:branch)$ prompt */
void lsh_print_prompt(void);

/* Formats a PS1 escape sequence into destination buffer. Returns 0 on success. */
int format_ps1(const char *ps1, char *out, size_t out_cap);

#endif /* APEX_PROMPT_H */
