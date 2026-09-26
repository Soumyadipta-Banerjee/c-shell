#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

static Job *job_list = NULL;
static int next_job_id = 1;

void jobs_init(void)
{
    job_list = NULL;
    next_job_id = 1;
}

int jobs_add(pid_t pid, const char *cmd_name)
{
    Job *j = malloc(sizeof(Job));
    if (!j)
    {
        perror("lsh: malloc job");
        return -1;
    }
    j->id = next_job_id++;
    j->pid = pid;
    j->cmd_name = strdup(cmd_name);
    j->next = NULL;

    // Append to end of job_list so jobs stay in order of creation
    if (job_list == NULL)
    {
        job_list = j;
    }
    else
    {
        Job *curr = job_list;
        while (curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = j;
    }

    printf("[%d] %d\n", j->id, (int)pid);
    fflush(stdout);
    return j->id;
}

void jobs_reap(void)
{
    Job **curr = &job_list;
    while (*curr != NULL)
    {
        int status;
        pid_t res = waitpid((*curr)->pid, &status, WNOHANG);
        if (res > 0)
        {
            printf("[%d]+  Done                    %s\n", (*curr)->id, (*curr)->cmd_name);
            fflush(stdout);
            Job *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free->cmd_name);
            free(to_free);
        }
        else if (res < 0 && errno == ECHILD)
        {
            Job *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free->cmd_name);
            free(to_free);
        }
        else
        {
            curr = &((*curr)->next);
        }
    }

    if (job_list == NULL)
    {
        next_job_id = 1;
    }
}

void jobs_list(void)
{
    Job *j = job_list;
    while (j != NULL)
    {
        printf("[%d]  Running                 %s\n", j->id, j->cmd_name);
        j = j->next;
    }
    fflush(stdout);
}

void jobs_cleanup(void)
{
    Job *curr = job_list;
    while (curr != NULL)
    {
        Job *next = curr->next;
        free(curr->cmd_name);
        free(curr);
        curr = next;
    }
    job_list = NULL;
    next_job_id = 1;
}

int jobs_count(void)
{
    jobs_reap();
    int count = 0;
    Job *curr = job_list;
    while (curr != NULL)
    {
        count++;
        curr = curr->next;
    }
    return count;
}

int lsh_jobs(char **args)
{
    (void)args;
    jobs_reap();
    jobs_list();
    return 0;
}
