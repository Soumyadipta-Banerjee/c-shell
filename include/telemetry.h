#ifndef APEX_TELEMETRY_H
#define APEX_TELEMETRY_H

#include <sys/types.h>

/* Execute command or pipeline with full getrusage telemetry and timing */
int lsh_execute_timed(char **args, int is_bg);

/* Built-in system info and resource dashboard */
int lsh_sysinfo(char **args);

#endif /* APEX_TELEMETRY_H */
