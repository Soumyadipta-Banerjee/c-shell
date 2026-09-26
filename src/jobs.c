#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

static Job *job_list = NULL;
static int next_job_id = 1;

void jobs_init(void)
{
    job_list = NULL;
    next_job_id = 1;
}

static Job *create_job_node(pid_t pid, const char *cmd_name, JobStatus status)
{
    Job *j = malloc(sizeof(Job));
    if (!j)
    {
        perror("apex-shell: malloc job");
        return NULL;
    }
    j->id = next_job_id++;
    j->pid = pid;
    j->cmd_name = strdup(cmd_name);
    j->status = status;
    j->next = NULL;
    return j;
}

static void append_job_node(Job *j)
{
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
}

int jobs_add(pid_t pid, const char *cmd_name)
{
    Job *j = create_job_node(pid, cmd_name, JOB_RUNNING);
    if (!j)
    {
        return -1;
    }
    append_job_node(j);
    printf("[%d] %d\n", j->id, (int)pid);
    fflush(stdout);
    return j->id;
}

int jobs_add_stopped(pid_t pid, const char *cmd_name)
{
    // Check if this pid is already in the job list
    Job *existing = jobs_find_by_pid(pid);
    if (existing)
    {
        existing->status = JOB_STOPPED;
        return existing->id;
    }

    Job *j = create_job_node(pid, cmd_name, JOB_STOPPED);
    if (!j)
    {
        return -1;
    }
    append_job_node(j);
    return j->id;
}

void jobs_reap(void)
{
    Job **curr = &job_list;
    while (*curr != NULL)
    {
        int status;
        pid_t res = waitpid((*curr)->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (res > 0)
        {
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                printf("[%d]+  Done                    %s\n", (*curr)->id, (*curr)->cmd_name);
                fflush(stdout);
                Job *to_free = *curr;
                *curr = (*curr)->next;
                free(to_free->cmd_name);
                free(to_free);
                continue;
            }
            else if (WIFSTOPPED(status))
            {
                (*curr)->status = JOB_STOPPED;
                printf("[%d]+  Stopped                 %s\n", (*curr)->id, (*curr)->cmd_name);
                fflush(stdout);
            }
            else if (WIFCONTINUED(status))
            {
                (*curr)->status = JOB_RUNNING;
            }
            curr = &((*curr)->next);
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
        const char *state_str = (j->status == JOB_STOPPED) ? "Stopped" : "Running";
        char marker = (j->next == NULL) ? '+' : '-';
        printf("[%d]%c %-24s%s\n", j->id, marker, state_str, j->cmd_name);
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

Job *jobs_find_by_id(int id)
{
    Job *curr = job_list;
    while (curr != NULL)
    {
        if (curr->id == id)
        {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

Job *jobs_find_by_pid(pid_t pid)
{
    Job *curr = job_list;
    while (curr != NULL)
    {
        if (curr->pid == pid)
        {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

Job *jobs_get_latest(void)
{
    if (job_list == NULL)
    {
        return NULL;
    }
    Job *curr = job_list;
    while (curr->next != NULL)
    {
        curr = curr->next;
    }
    return curr;
}

Job *jobs_get_latest_stopped(void)
{
    Job *last_stopped = NULL;
    Job *curr = job_list;
    while (curr != NULL)
    {
        if (curr->status == JOB_STOPPED)
        {
            last_stopped = curr;
        }
        curr = curr->next;
    }
    return last_stopped;
}

void jobs_remove(pid_t pid)
{
    Job **curr = &job_list;
    while (*curr != NULL)
    {
        if ((*curr)->pid == pid)
        {
            Job *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free->cmd_name);
            free(to_free);
            break;
        }
        curr = &((*curr)->next);
    }
    if (job_list == NULL)
    {
        next_job_id = 1;
    }
}

void jobs_set_status(pid_t pid, JobStatus status)
{
    Job *j = jobs_find_by_pid(pid);
    if (j)
    {
        j->status = status;
    }
}

int lsh_jobs(char **args)
{
    (void)args;
    jobs_reap();
    jobs_list();
    return 0;
}

int lsh_fg(char **args)
{
    jobs_reap();
    Job *j = NULL;
    if (args[1] == NULL)
    {
        j = jobs_get_latest_stopped();
        if (!j)
        {
            j = jobs_get_latest();
        }
    }
    else
    {
        const char *arg = args[1];
        if (arg[0] == '%')
        {
            arg++;
        }
        int id = atoi(arg);
        j = jobs_find_by_id(id);
    }

    if (!j)
    {
        fprintf(stderr, "apex-shell: fg: current: no such job\n");
        return 1;
    }

    printf("%s\n", j->cmd_name);
    fflush(stdout);

    pid_t pid = j->pid;
    if (j->status == JOB_STOPPED)
    {
        if (kill(-pid, SIGCONT) < 0)
        {
            kill(pid, SIGCONT);
        }
        j->status = JOB_RUNNING;
    }

    if (isatty(STDIN_FILENO))
    {
        tcsetpgrp(STDIN_FILENO, pid);
    }

    int status;
    do
    {
        waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));

    if (isatty(STDIN_FILENO))
    {
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }

    if (WIFSTOPPED(status))
    {
        Job *existing = jobs_find_by_pid(pid);
        if (existing)
        {
            existing->status = JOB_STOPPED;
            printf("\n[%d]+  Stopped                 %s\n", existing->id, existing->cmd_name);
            fflush(stdout);
        }
        return 128 + WSTOPSIG(status);
    }
    else
    {
        int exit_code = 0;
        if (WIFEXITED(status))
        {
            exit_code = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            exit_code = 128 + WTERMSIG(status);
        }
        jobs_remove(pid);
        return exit_code;
    }
}

int lsh_bg(char **args)
{
    jobs_reap();
    Job *j = NULL;
    if (args[1] == NULL)
    {
        j = jobs_get_latest_stopped();
        if (!j)
        {
            j = jobs_get_latest();
        }
    }
    else
    {
        const char *arg = args[1];
        if (arg[0] == '%')
        {
            arg++;
        }
        int id = atoi(arg);
        j = jobs_find_by_id(id);
    }

    if (!j)
    {
        fprintf(stderr, "apex-shell: bg: current: no such job\n");
        return 1;
    }

    if (kill(-j->pid, SIGCONT) < 0)
    {
        kill(j->pid, SIGCONT);
    }

    j->status = JOB_RUNNING;
    printf("[%d]+ %s &\n", j->id, j->cmd_name);
    fflush(stdout);
    return 0;
}

int lsh_kill(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "apex-shell: kill: usage: kill [-s sig | -sig] pid | %%job_id ...\n");
        return 1;
    }

    int sig = SIGTERM;
    int idx = 1;

    if (args[idx][0] == '-')
    {
        const char *signame = args[idx] + 1;
        if (strcmp(signame, "9") == 0 || strcasecmp(signame, "KILL") == 0)
        {
            sig = SIGKILL;
        }
        else if (strcmp(signame, "15") == 0 || strcasecmp(signame, "TERM") == 0)
        {
            sig = SIGTERM;
        }
        else if (strcmp(signame, "STOP") == 0)
        {
            sig = SIGSTOP;
        }
        else if (strcmp(signame, "CONT") == 0)
        {
            sig = SIGCONT;
        }
        else if (strcmp(signame, "INT") == 0 || strcmp(signame, "2") == 0)
        {
            sig = SIGINT;
        }
        else
        {
            sig = atoi(signame);
            if (sig <= 0)
            {
                fprintf(stderr, "apex-shell: kill: invalid signal specification: %s\n", args[idx]);
                return 1;
            }
        }
        idx++;
    }

    if (args[idx] == NULL)
    {
        fprintf(stderr, "apex-shell: kill: usage: kill [-s sig | -sig] pid | %%job_id ...\n");
        return 1;
    }

    int ret = 0;
    while (args[idx] != NULL)
    {
        pid_t pid = 0;
        if (args[idx][0] == '%')
        {
            int jid = atoi(args[idx] + 1);
            Job *j = jobs_find_by_id(jid);
            if (!j)
            {
                fprintf(stderr, "apex-shell: kill: %s: no such job\n", args[idx]);
                ret = 1;
                idx++;
                continue;
            }
            pid = j->pid;
        }
        else
        {
            pid = (pid_t)atoi(args[idx]);
        }

        if (kill(pid, sig) < 0)
        {
            perror("apex-shell: kill");
            ret = 1;
        }
        idx++;
    }
    return ret;
}

