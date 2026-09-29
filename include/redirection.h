#ifndef APEX_REDIRECTION_H
#define APEX_REDIRECTION_H

/* Evaluates and applies I/O redirections (<, >, >>, 2>, 2>>, &>, 2>&1, 1>&2),
   compacting the args array in-place to strip redirection tokens. */
void handle_redirection(char **args);

#endif /* APEX_REDIRECTION_H */
