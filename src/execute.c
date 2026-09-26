#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "execute.h"
#include "builtins.h"
#include "parser.h"
#include "jobs.h"
#include "telemetry.h"
#include "fuzzy.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

int g_last_exit_status = 0;
int g_should_exit = 0;

static char *build_cmd_str(char **args)
{
    size_t len = 0;
    for (int i = 0; args[i] != NULL; i++)
    {
        len += strlen(args[i]) + 1;
    }
    len += 4;
    char *res = malloc(len);
    if (!res)
    {
        return strdup("job &");
    }
    res[0] = '\0';
    for (int i = 0; args[i] != NULL; i++)
    {
        strcat(res, args[i]);
        if (args[i + 1] != NULL)
        {
            strcat(res, " ");
        }
    }
    strcat(res, " &");
    return res;
}

static void handle_redirection(char **args)
{
    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;
    int i = 0, j = 0;

    while (args[i] != NULL)
    {
        if (strcmp(args[i], "<") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            input_file = args[i + 1];
            i += 2;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            output_file = args[i + 1];
            append = 0;
            i += 2;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token 'newline'\n");
                exit(EXIT_FAILURE);
            }
            output_file = args[i + 1];
            append = 1;
            i += 2;
        }
        else
        {
            args[j++] = args[i++];
        }
    }
    args[j] = NULL;

    if (input_file)
    {
        int in_fd = open(input_file, O_RDONLY);
        if (in_fd < 0)
        {
            perror("lsh: input redirection");
            exit(EXIT_FAILURE);
        }
        if (dup2(in_fd, STDIN_FILENO) < 0)
        {
            perror("lsh: dup2 input");
            exit(EXIT_FAILURE);
        }
        close(in_fd);
    }

    if (output_file)
    {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        int out_fd = open(output_file, flags, 0644);
        if (out_fd < 0)
        {
            perror("lsh: output redirection");
            exit(EXIT_FAILURE);
        }
        if (dup2(out_fd, STDOUT_FILENO) < 0)
        {
            perror("lsh: dup2 output");
            exit(EXIT_FAILURE);
        }
        close(out_fd);
    }
}

int lsh_launch(char **args, int is_bg)
{
    pid_t pid;
    int status;

    struct sigaction sa_ignore, sa_orig_int, sa_orig_tstp;
    if (!is_bg)
    {
        memset(&sa_ignore, 0, sizeof(sa_ignore));
        sa_ignore.sa_handler = SIG_IGN;
        sigemptyset(&sa_ignore.sa_mask);
        sa_ignore.sa_flags = 0;
        sigaction(SIGINT, &sa_ignore, &sa_orig_int);
        sigaction(SIGTSTP, &sa_ignore, &sa_orig_tstp);
    }

    pid = fork();
    if (pid == 0)
    {
        if (is_bg)
        {
            signal(SIGINT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
        }
        else
        {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
        }

        handle_redirection(args);
        if (args[0] == NULL)
        {
            exit(EXIT_SUCCESS);
        }
        if (execvp(args[0], args) == -1)
        {
            if (errno == ENOENT)
            {
                print_command_not_found(args[0]);
                exit(127);
            }
            else
            {
                perror("apex-shell");
                exit(126);
            }
        }
    }
    else if (pid < 0)
    {
        perror("lsh: fork");
        if (!is_bg)
        {
            sigaction(SIGINT, &sa_orig_int, NULL);
            sigaction(SIGTSTP, &sa_orig_tstp, NULL);
        }
        return 1;
    }
    else
    {
        if (is_bg)
        {
            char *cmd_str = build_cmd_str(args);
            jobs_add(pid, cmd_str);
            free(cmd_str);
            return 0;
        }

        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));

        if (WIFSIGNALED(status))
        {
            if (WTERMSIG(status) == SIGINT)
            {
                write(STDOUT_FILENO, "\n", 1);
            }
        }
        else if (WIFSTOPPED(status))
        {
            printf("\n[%d] Stopped\n", pid);
        }

        sigaction(SIGINT, &sa_orig_int, NULL);
        sigaction(SIGTSTP, &sa_orig_tstp, NULL);

        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            return 128 + WTERMSIG(status);
        }
    }
    return 1;
}

int lsh_execute_pipeline(char ***cmd_args, int num_cmds, int is_bg)
{
    int num_pipes = num_cmds - 1;
    int pipefds[2 * num_pipes];

    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(pipefds + i * 2) < 0)
        {
            perror("lsh: pipe");
            return 1;
        }
    }

    pid_t *pids = malloc(sizeof(pid_t) * num_cmds);
    if (!pids)
    {
        fprintf(stderr, "lsh: allocation error\n");
        return 1;
    }

    struct sigaction sa_ignore, sa_orig_int, sa_orig_tstp;
    if (!is_bg)
    {
        memset(&sa_ignore, 0, sizeof(sa_ignore));
        sa_ignore.sa_handler = SIG_IGN;
        sigemptyset(&sa_ignore.sa_mask);
        sa_ignore.sa_flags = 0;
        sigaction(SIGINT, &sa_ignore, &sa_orig_int);
        sigaction(SIGTSTP, &sa_ignore, &sa_orig_tstp);
    }

    for (int i = 0; i < num_cmds; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            if (is_bg)
            {
                signal(SIGINT, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
            }
            else
            {
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
            }

            if (i > 0)
            {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0)
                {
                    perror("lsh: dup2 stdin");
                    exit(EXIT_FAILURE);
                }
            }
            if (i < num_cmds - 1)
            {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0)
                {
                    perror("lsh: dup2 stdout");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < 2 * num_pipes; j++)
            {
                close(pipefds[j]);
            }

            handle_redirection(cmd_args[i]);

            if (cmd_args[i][0] == NULL)
            {
                exit(EXIT_SUCCESS);
            }

            for (int b = 0; b < lsh_num_builtins(); b++)
            {
                if (strcmp(cmd_args[i][0], builtin_str[b]) == 0)
                {
                    int res = (*builtin_func[b])(cmd_args[i]);
                    exit(res == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
                }
            }

            if (execvp(cmd_args[i][0], cmd_args[i]) == -1)
            {
                if (errno == ENOENT)
                {
                    print_command_not_found(cmd_args[i][0]);
                    exit(127);
                }
                else
                {
                    perror("apex-shell");
                    exit(126);
                }
            }
        }
        else if (pids[i] < 0)
        {
            perror("lsh: fork");
            free(pids);
            if (!is_bg)
            {
                sigaction(SIGINT, &sa_orig_int, NULL);
                sigaction(SIGTSTP, &sa_orig_tstp, NULL);
            }
            return 1;
        }
    }

    for (int i = 0; i < 2 * num_pipes; i++)
    {
        close(pipefds[i]);
    }

    if (is_bg)
    {
        char *cmd_str = build_cmd_str(cmd_args[0]);
        jobs_add(pids[num_cmds - 1], cmd_str);
        free(cmd_str);
        free(pids);
        return 0;
    }

    int any_signaled = 0;
    int last_status = 0;
    for (int i = 0; i < num_cmds; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == num_cmds - 1)
        {
            if (WIFEXITED(status))
            {
                last_status = WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status))
            {
                last_status = 128 + WTERMSIG(status);
            }
        }
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
        {
            any_signaled = 1;
        }
    }

    if (any_signaled)
    {
        write(STDOUT_FILENO, "\n", 1);
    }

    free(pids);
    sigaction(SIGINT, &sa_orig_int, NULL);
    sigaction(SIGTSTP, &sa_orig_tstp, NULL);
    return last_status;
}

int lsh_execute(char **args, int is_bg)
{
    if (args[0] == NULL)
    {
        return 0;
    }

    if (strcmp(args[0], "time") == 0)
    {
        if (args[1] == NULL)
        {
            fprintf(stderr, "time: missing command to profile\nUsage: time <command> [args...]\n");
            return 1;
        }
        return lsh_execute_timed(args + 1, is_bg);
    }

    int num_cmds = 1;
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            num_cmds++;
        }
    }

    if (num_cmds > 1)
    {
        char ***cmd_args = malloc(sizeof(char **) * num_cmds);
        if (!cmd_args)
        {
            fprintf(stderr, "lsh: allocation error\n");
            return 1;
        }

        int cmd_idx = 0;
        cmd_args[0] = &args[0];

        for (int i = 0; args[i] != NULL; i++)
        {
            if (strcmp(args[i], "|") == 0)
            {
                args[i] = NULL;
                cmd_idx++;
                cmd_args[cmd_idx] = &args[i + 1];
            }
        }

        for (int i = 0; i < num_cmds; i++)
        {
            if (cmd_args[i][0] == NULL)
            {
                fprintf(stderr, "lsh: syntax error near unexpected token '|'\n");
                free(cmd_args);
                return 1;
            }
        }

        int status = lsh_execute_pipeline(cmd_args, num_cmds, is_bg);
        free(cmd_args);
        return status;
    }

    for (int i = 0; i < lsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            if (is_bg)
            {
                pid_t pid = fork();
                if (pid == 0)
                {
                    int res = (*builtin_func[i])(args);
                    exit(res == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
                }
                else if (pid > 0)
                {
                    char *cmd_str = build_cmd_str(args);
                    jobs_add(pid, cmd_str);
                    free(cmd_str);
                    return 0;
                }
                perror("lsh: fork");
                return 1;
            }
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args, is_bg);
}

int lsh_execute_line(ShellToken **tokens)
{
    int i = 0;
    int execute_this_cmd = 1;

    while (tokens[i] != NULL && !g_should_exit)
    {
        int cmd_start = i;
        char *connector = NULL;

        while (tokens[i] != NULL)
        {
            if (strcmp(tokens[i]->text, ";") == 0 ||
                strcmp(tokens[i]->text, "&&") == 0 ||
                strcmp(tokens[i]->text, "||") == 0 ||
                strcmp(tokens[i]->text, "&") == 0)
            {
                connector = tokens[i]->text;
                i++;
                break;
            }
            i++;
        }

        int cmd_end = connector ? (i - 1) : i;
        int cmd_len = cmd_end - cmd_start;

        if (cmd_len > 0)
        {
            int is_bg = (connector != NULL && strcmp(connector, "&") == 0);

            if (execute_this_cmd)
            {
                char **args = malloc((cmd_len + 1) * sizeof(char *));
                if (!args)
                {
                    fprintf(stderr, "lsh: allocation error\n");
                    return 1;
                }

                for (int j = 0; j < cmd_len; j++)
                {
                    args[j] = expand_token(tokens[cmd_start + j]);
                }
                args[cmd_len] = NULL;

                g_last_exit_status = lsh_execute(args, is_bg);

                for (int j = 0; j < cmd_len; j++)
                {
                    free(args[j]);
                }
                free(args);
            }
        }

        if (connector != NULL)
        {
            if (strcmp(connector, ";") == 0 || strcmp(connector, "&") == 0)
            {
                execute_this_cmd = 1;
            }
            else if (strcmp(connector, "&&") == 0)
            {
                execute_this_cmd = (g_last_exit_status == 0);
            }
            else if (strcmp(connector, "||") == 0)
            {
                execute_this_cmd = (g_last_exit_status != 0);
            }
        }
    }

    return g_last_exit_status;
}
