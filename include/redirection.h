#ifndef APEX_REDIRECTION_H
#define APEX_REDIRECTION_H

#include "parser.h"

/* Evaluates and applies I/O redirections (<, >, >>, 2>, 2>>, &>, 2>&1, 1>&2, <<<),
   compacting the args array in-place to strip redirection tokens. */
void handle_redirection(char **args);

/* Scans tokens for heredoc operators (<< DELIM) and uses read_line_cb to collect
   the multi-line body, transforming << DELIM into <<< "captured body". */
int resolve_heredocs(ShellToken **tokens, char *(*read_line_cb)(void *ctx), void *ctx);

#endif /* APEX_REDIRECTION_H */
