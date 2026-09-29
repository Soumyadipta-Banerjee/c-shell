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
int lsh_sysinfo(char **args);
int lsh_history(char **args);
int lsh_alias(char **args);
int lsh_unalias(char **args);
int lsh_safemode(char **args);
int lsh_fg(char **args);
int lsh_bg(char **args);
int lsh_kill(char **args);
int lsh_source(char **args);
int lsh_pushd(char **args);
int lsh_popd(char **args);
int lsh_dirs(char **args);
int lsh_z(char **args);
int lsh_set(char **args);

int lsh_num_builtins(void);

extern char *builtin_str[];
extern int (*builtin_func[])(char **);

#endif /* CSHELL_BUILTINS_H */
