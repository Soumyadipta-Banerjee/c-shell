#ifndef CSHELL_JOBS_H
#define CSHELL_JOBS_H

#include <sys/types.h>

typedef struct Job {
    int id;
    pid_t pid;
    char *cmd_name;
    struct Job *next;
} Job;

/* Initializes job tracking */
void jobs_init(void);

/* Adds a new background job and prints [id] pid */
int jobs_add(pid_t pid, const char *cmd_name);

/* Reaps completed background jobs non-blockingly */
void jobs_reap(void);

/* Prints the list of active background jobs */
void jobs_list(void);

/* Cleans up remaining job data on exit */
void jobs_cleanup(void);

/* Returns the number of currently active jobs */
int jobs_count(void);

/* Built-in 'jobs' command */
int lsh_jobs(char **args);

#endif /* CSHELL_JOBS_H */
