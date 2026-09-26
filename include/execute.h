#ifndef CSHELL_EXECUTE_H
#define CSHELL_EXECUTE_H

#include "parser.h"

/* Global shell execution state */
extern int g_last_exit_status;
extern int g_should_exit;

/* Executes a command line sequence separated by ;, &&, ||, & */
int lsh_execute_line(ShellToken **tokens);

/* Executes a single command or pipeline stage, supporting background execution */
int lsh_execute(char **args, int is_bg);

/* Launches a single external command with redirection */
int lsh_launch(char **args, int is_bg);

/* Executes an arbitrary multi-stage pipeline */
int lsh_execute_pipeline(char ***cmd_args, int num_cmds, int is_bg);

#endif /* CSHELL_EXECUTE_H */
