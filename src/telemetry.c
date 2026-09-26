#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "telemetry.h"
#include "execute.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

int lsh_execute_timed(char **args, int is_bg)
{
    struct timespec ts_start, ts_end;
    struct rusage ru_ch_start, ru_ch_end;
    struct rusage ru_self_start, ru_self_end;

    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    getrusage(RUSAGE_CHILDREN, &ru_ch_start);
    getrusage(RUSAGE_SELF, &ru_self_start);

    int status = lsh_execute(args, is_bg);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    getrusage(RUSAGE_CHILDREN, &ru_ch_end);
    getrusage(RUSAGE_SELF, &ru_self_end);

    double wall_sec = (double)(ts_end.tv_sec - ts_start.tv_sec) +
                      (double)(ts_end.tv_nsec - ts_start.tv_nsec) / 1000000000.0;

    double ch_user_sec = (double)(ru_ch_end.ru_utime.tv_sec - ru_ch_start.ru_utime.tv_sec) +
                         (double)(ru_ch_end.ru_utime.tv_usec - ru_ch_start.ru_utime.tv_usec) / 1000000.0;
    double ch_sys_sec  = (double)(ru_ch_end.ru_stime.tv_sec - ru_ch_start.ru_stime.tv_sec) +
                         (double)(ru_ch_end.ru_stime.tv_usec - ru_ch_start.ru_stime.tv_usec) / 1000000.0;

    double self_user_sec = (double)(ru_self_end.ru_utime.tv_sec - ru_self_start.ru_utime.tv_sec) +
                          (double)(ru_self_end.ru_utime.tv_usec - ru_self_start.ru_utime.tv_usec) / 1000000.0;
    double self_sys_sec  = (double)(ru_self_end.ru_stime.tv_sec - ru_self_start.ru_stime.tv_sec) +
                          (double)(ru_self_end.ru_stime.tv_usec - ru_self_start.ru_stime.tv_usec) / 1000000.0;

    double user_sec = (ch_user_sec > 0.00001) ? ch_user_sec : self_user_sec;
    double sys_sec  = (ch_sys_sec > 0.00001) ? ch_sys_sec : self_sys_sec;

    long minflt = (ru_ch_end.ru_minflt - ru_ch_start.ru_minflt);
    long majflt = (ru_ch_end.ru_majflt - ru_ch_start.ru_majflt);
    if (minflt == 0 && majflt == 0)
    {
        minflt = (ru_self_end.ru_minflt - ru_self_start.ru_minflt);
        majflt = (ru_self_end.ru_majflt - ru_self_start.ru_majflt);
    }

    long nvcsw  = (ru_ch_end.ru_nvcsw - ru_ch_start.ru_nvcsw);
    long nivcsw = (ru_ch_end.ru_nivcsw - ru_ch_start.ru_nivcsw);
    if (nvcsw == 0 && nivcsw == 0)
    {
        nvcsw  = (ru_self_end.ru_nvcsw - ru_self_start.ru_nvcsw);
        nivcsw = (ru_self_end.ru_nivcsw - ru_self_start.ru_nivcsw);
    }

    long peak_rss_kb = ru_ch_end.ru_maxrss > ru_self_end.ru_maxrss ? ru_ch_end.ru_maxrss : ru_self_end.ru_maxrss;
    char ram_buf[32];
    if (peak_rss_kb >= 1024)
    {
        snprintf(ram_buf, sizeof(ram_buf), "%.2f MB", (double)peak_rss_kb / 1024.0);
    }
    else
    {
        snprintf(ram_buf, sizeof(ram_buf), "%ld KB", peak_rss_kb);
    }

    char wall_buf[32];
    snprintf(wall_buf, sizeof(wall_buf), "%.4fs", wall_sec);

    char line1[128], line2[128], line3[128];
    snprintf(line1, sizeof(line1), "Wall Time: %-10s | CPU: %.4fs user, %.4fs sys",
             wall_buf, user_sec, sys_sec);
    snprintf(line2, sizeof(line2), "Peak RAM:  %-10s | Page Faults: %ld minor, %ld major",
             ram_buf, minflt, majflt);
    snprintf(line3, sizeof(line3), "Context Switches: %ld voluntary, %ld involuntary",
             nvcsw, nivcsw);

    // Output Telemetry to stderr (exactly 66 columns wide)
    fprintf(stderr, "\n┌─ Execution Telemetry ──────────────────────────────────────────┐\n");
    fprintf(stderr, "│ %-62s │\n", line1);
    fprintf(stderr, "│ %-62s │\n", line2);
    fprintf(stderr, "│ %-62s │\n", line3);
    fprintf(stderr, "└────────────────────────────────────────────────────────────────┘\n");
    fflush(stderr);

    return status;
}

int lsh_sysinfo(char **args)
{
    (void)args;

    // 1. CPU & Load Averages
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores < 1)
    {
        num_cores = 1;
    }

    double load1 = 0.0, load5 = 0.0, load15 = 0.0;
    FILE *f_load = fopen("/proc/loadavg", "r");
    if (f_load)
    {
        if (fscanf(f_load, "%lf %lf %lf", &load1, &load5, &load15) != 3)
        {
            load1 = load5 = load15 = 0.0;
        }
        fclose(f_load);
    }

    // 2. Memory Info
    unsigned long long mem_total = 0, mem_free = 0, mem_avail = 0;
    unsigned long long swap_total = 0, swap_free = 0;
    FILE *f_mem = fopen("/proc/meminfo", "r");
    if (f_mem)
    {
        char line[256];
        while (fgets(line, sizeof(line), f_mem))
        {
            if (strncmp(line, "MemTotal:", 9) == 0)
            {
                sscanf(line + 9, "%llu", &mem_total);
            }
            else if (strncmp(line, "MemFree:", 8) == 0)
            {
                sscanf(line + 8, "%llu", &mem_free);
            }
            else if (strncmp(line, "MemAvailable:", 13) == 0)
            {
                sscanf(line + 13, "%llu", &mem_avail);
            }
            else if (strncmp(line, "SwapTotal:", 10) == 0)
            {
                sscanf(line + 10, "%llu", &swap_total);
            }
            else if (strncmp(line, "SwapFree:", 9) == 0)
            {
                sscanf(line + 9, "%llu", &swap_free);
            }
        }
        fclose(f_mem);
    }

    if (mem_avail == 0)
    {
        mem_avail = mem_free;
    }

    unsigned long long mem_used = (mem_total > mem_avail) ? (mem_total - mem_avail) : 0;
    double mem_pct = (mem_total > 0) ? ((double)mem_used * 100.0 / (double)mem_total) : 0.0;

    unsigned long long swap_used = (swap_total > swap_free) ? (swap_total - swap_free) : 0;
    double swap_pct = (swap_total > 0) ? ((double)swap_used * 100.0 / (double)swap_total) : 0.0;

    // 3. System Uptime
    double uptime_sec = 0.0;
    FILE *f_up = fopen("/proc/uptime", "r");
    if (f_up)
    {
        if (fscanf(f_up, "%lf", &uptime_sec) != 1)
        {
            uptime_sec = 0.0;
        }
        fclose(f_up);
    }
    long up_total = (long)uptime_sec;
    long up_days = up_total / 86400;
    long up_hours = (up_total % 86400) / 3600;
    long up_mins = (up_total % 3600) / 60;
    long up_secs = up_total % 60;

    // 4. Shell Process Telemetry
    pid_t shell_pid = getpid();
    struct rusage ru_shell;
    getrusage(RUSAGE_SELF, &ru_shell);
    long shell_rss = ru_shell.ru_maxrss;
    int active_jobs = jobs_count();

    char line_cpu[128], line_mem[128], line_swap[128], line_sys[128], line_shell[128];
    snprintf(line_cpu, sizeof(line_cpu), "CPU:        %ld Cores | Load: %.2f (1m), %.2f (5m), %.2f (15m)",
             num_cores, load1, load5, load15);
    snprintf(line_mem, sizeof(line_mem), "Memory:     %llu MB / %llu MB (%.1f%% used)",
             mem_used / 1024, mem_total / 1024, mem_pct);
    snprintf(line_swap, sizeof(line_swap), "Swap:       %llu MB / %llu MB (%.1f%% used)",
             swap_used / 1024, swap_total / 1024, swap_pct);
    if (up_days > 0)
    {
        snprintf(line_sys, sizeof(line_sys), "System:     Uptime: %ldd %ldh %ldm %lds",
                 up_days, up_hours, up_mins, up_secs);
    }
    else
    {
        snprintf(line_sys, sizeof(line_sys), "System:     Uptime: %ldh %ldm %lds",
                 up_hours, up_mins, up_secs);
    }
    snprintf(line_shell, sizeof(line_shell), "Apex Shell: PID %d | Peak RSS: %.2f MB | Active Jobs: %d",
             (int)shell_pid, (double)shell_rss / 1024.0, active_jobs);

    // 5. Output System Dashboard (exactly 66 columns wide)
    printf("┌── Apex System Observability ──────────────────────────────────┐\n");
    printf("│ %-62s │\n", line_cpu);
    printf("│ %-62s │\n", line_mem);
    printf("│ %-62s │\n", line_swap);
    printf("│ %-62s │\n", line_sys);
    printf("│ %-62s │\n", line_shell);
    printf("└────────────────────────────────────────────────────────────────┘\n");
    fflush(stdout);

    return 0;
}
