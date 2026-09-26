#ifndef CSHELL_BUILTINS_H
#define CSHELL_BUILTINS_H

/* Built-in shell commands */
int lsh_cd(char **args);
int lsh_pwd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_jobs(char **args);
int lsh_export(char **args);
int lsh_unset(char **args);
int lsh_env(char **args);

int lsh_num_builtins(void);

extern char *builtin_str[];
extern int (*builtin_func[])(char **);

#endif /* CSHELL_BUILTINS_H */
