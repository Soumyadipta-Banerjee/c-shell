#ifndef CSHELL_JOBS_H
#define CSHELL_JOBS_H

#include <sys/types.h>

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED
} JobStatus;

typedef struct Job {
    int id;
    pid_t pid;
    char *cmd_name;
    JobStatus status;
    struct Job *next;
} Job;

/* Initializes job tracking */
void jobs_init(void);

/* Adds a new background job in running state */
int jobs_add(pid_t pid, const char *cmd_name);

/* Adds a suspended foreground job in stopped state */
int jobs_add_stopped(pid_t pid, const char *cmd_name);

/* Reaps completed background jobs non-blockingly */
void jobs_reap(void);

/* Prints the list of active background and stopped jobs */
void jobs_list(void);

/* Cleans up remaining job data on exit */
void jobs_cleanup(void);

/* Returns the number of currently active jobs */
int jobs_count(void);

/* Job lookup and status helpers */
Job *jobs_find_by_id(int id);
Job *jobs_find_by_pid(pid_t pid);
Job *jobs_get_latest(void);
Job *jobs_get_latest_stopped(void);
void jobs_remove(pid_t pid);
void jobs_set_status(pid_t pid, JobStatus status);

/* Built-in job commands */
int lsh_jobs(char **args);
int lsh_fg(char **args);
int lsh_bg(char **args);
int lsh_kill(char **args);

#endif /* CSHELL_JOBS_H */
