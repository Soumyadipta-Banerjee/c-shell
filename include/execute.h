#ifndef CSHELL_EXECUTE_H
#define CSHELL_EXECUTE_H

#include "parser.h"

/* Global shell execution state */
extern int g_last_exit_status;
extern int g_should_exit;

/* Executes a command line sequence separated by ;, &&, || */
int lsh_execute_line(ShellToken **tokens);

/* Executes a single command or pipeline stage */
int lsh_execute(char **args);

/* Launches a single external command with redirection */
int lsh_launch(char **args);

/* Executes an arbitrary multi-stage pipeline */
int lsh_execute_pipeline(char ***cmd_args, int num_cmds);

#endif /* CSHELL_EXECUTE_H */
