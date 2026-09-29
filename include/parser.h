#ifndef CSHELL_PARSER_H
#define CSHELL_PARSER_H

typedef struct {
    char *text;
    int is_literal;
} ShellToken;

#include "prompt.h"
#include "expander.h"

/* Reads a line from stdin with EOF and EINTR handling */
char *lsh_read_line(void);

/* Splits a command line into structured tokens */
ShellToken **lsh_split_line(char *line);

/* Frees tokens array and inner string buffers */
void lsh_free_tokens(ShellToken **tokens);

#endif /* CSHELL_PARSER_H */
