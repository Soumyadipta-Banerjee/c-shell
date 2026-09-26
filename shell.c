#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

/* Global shell state */
volatile sig_atomic_t g_interrupted = 0;
int g_last_exit_status = 0;
int g_should_exit = 0;

static void sigint_handler(int signo)
{
    (void)signo;
    g_interrupted = 1;
    if (isatty(STDIN_FILENO))
    {
        write(STDOUT_FILENO, "\n", 1);
    }
}

void lsh_init_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    struct sigaction sa_tstp;
    memset(&sa_tstp, 0, sizeof(sa_tstp));
    sa_tstp.sa_handler = SIG_IGN;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    sigaction(SIGTSTP, &sa_tstp, NULL);
}

/* Built-in shell command declarations */
int lsh_cd(char **args);
int lsh_pwd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *builtin_str[] = {
    "cd",
    "pwd",
    "help",
    "exit"
};

int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_pwd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_builtins(void)
{
    return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
    const char *target = args[1];
    if (target == NULL || strcmp(target, "~") == 0)
    {
        target = getenv("HOME");
        if (target == NULL)
        {
            fprintf(stderr, "lsh: cd: HOME not set\n");
            return 1;
        }
    }
    if (chdir(target) != 0)
    {
        perror("lsh: cd");
        return 1;
    }
    return 0;
}

int lsh_pwd(char **args)
{
    (void)args;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
        return 0;
    }
    else
    {
        perror("lsh: pwd");
        return 1;
    }
}

int lsh_help(char **args)
{
    (void)args;
    printf("Soumya's C-Shell\n");
    printf("Type program names and arguments, then hit enter.\n\n");
    printf("Built-in commands:\n");
    for (int i = 0; i < lsh_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }
    printf("\nFeatures supported:\n");
    printf("  - Command Chaining: ; (seq), && (and), || (or)\n");
    printf("  - Pipelines:        cmd1 | cmd2 | ... | cmdN\n");
    printf("  - I/O Redirection:  < (input), > (output), >> (append)\n");
    printf("  - Quoted strings:   \"hello world\" or 'hello world'\n");
    printf("  - Expansions:       $VAR, $?\n");
    printf("  - Signal Handling:  Ctrl+C (SIGINT) and Ctrl+Z (SIGTSTP) protection\n");
    return 0;
}

int lsh_exit(char **args)
{
    g_should_exit = 1;
    if (args[1] != NULL)
    {
        g_last_exit_status = atoi(args[1]);
    }
    return g_last_exit_status;
}

void lsh_print_prompt(void)
{
    if (!isatty(STDIN_FILENO))
    {
        return;
    }

    char cwd[1024];
    char hostname[1024];
    char *user = getenv("USER");
    if (!user)
    {
        user = "user";
    }

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strncpy(hostname, "localhost", sizeof(hostname));
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char *home = getenv("HOME");
        char display_cwd[1024];
        if (home && strncmp(cwd, home, strlen(home)) == 0)
        {
            snprintf(display_cwd, sizeof(display_cwd), "~%s", cwd + strlen(home));
        }
        else
        {
            snprintf(display_cwd, sizeof(display_cwd), "%s", cwd);
        }
        // Bold green for user@host, bold blue for cwd
        printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", user, hostname, display_cwd);
    }
    else
    {
        printf("> ");
    }
    fflush(stdout);
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

int lsh_launch(char **args)
{
    pid_t pid;
    int status;

    struct sigaction sa_ignore, sa_orig_int, sa_orig_tstp;
    memset(&sa_ignore, 0, sizeof(sa_ignore));
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGINT, &sa_ignore, &sa_orig_int);
    sigaction(SIGTSTP, &sa_ignore, &sa_orig_tstp);

    pid = fork();
    if (pid == 0)
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        handle_redirection(args);
        if (args[0] == NULL)
        {
            exit(EXIT_SUCCESS);
        }
        if (execvp(args[0], args) == -1)
        {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("lsh: fork");
        sigaction(SIGINT, &sa_orig_int, NULL);
        sigaction(SIGTSTP, &sa_orig_tstp, NULL);
        return 1;
    }
    else
    {
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
    return 1;
}

int lsh_execute_pipeline(char ***cmd_args, int num_cmds)
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
    memset(&sa_ignore, 0, sizeof(sa_ignore));
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGINT, &sa_ignore, &sa_orig_int);
    sigaction(SIGTSTP, &sa_ignore, &sa_orig_tstp);

    for (int i = 0; i < num_cmds; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

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
                perror("lsh");
            }
            exit(EXIT_FAILURE);
        }
        else if (pids[i] < 0)
        {
            perror("lsh: fork");
            free(pids);
            sigaction(SIGINT, &sa_orig_int, NULL);
            sigaction(SIGTSTP, &sa_orig_tstp, NULL);
            return 1;
        }
    }

    for (int i = 0; i < 2 * num_pipes; i++)
    {
        close(pipefds[i]);
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

int lsh_execute(char **args)
{
    if (args[0] == NULL)
    {
        return 0;
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

        int status = lsh_execute_pipeline(cmd_args, num_cmds);
        free(cmd_args);
        return status;
    }

    for (int i = 0; i < lsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

typedef struct {
    char *text;
    int is_literal;
} ShellToken;

#define LSH_TOK_BUFSIZE 64

static char *expand_token(const ShellToken *tok)
{
    if (tok->is_literal)
    {
        return strdup(tok->text);
    }

    if (strcmp(tok->text, "$?") == 0)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", g_last_exit_status);
        return strdup(buf);
    }

    if (tok->text[0] == '$' && tok->text[1] != '\0')
    {
        char *val = getenv(tok->text + 1);
        if (val)
        {
            return strdup(val);
        }
        return strdup("");
    }

    return strdup(tok->text);
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
                strcmp(tokens[i]->text, "||") == 0)
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

                g_last_exit_status = lsh_execute(args);

                for (int j = 0; j < cmd_len; j++)
                {
                    free(args[j]);
                }
                free(args);
            }
        }

        if (connector != NULL)
        {
            if (strcmp(connector, ";") == 0)
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

ShellToken **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    ShellToken **tokens = malloc(bufsize * sizeof(ShellToken *));
    char *p = line;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (*p != '\0')
    {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        {
            p++;
        }
        if (*p == '\0')
        {
            break;
        }

        ShellToken *tok = malloc(sizeof(ShellToken));
        if (!tok)
        {
            fprintf(stderr, "lsh: allocation error\n");
            exit(EXIT_FAILURE);
        }

        if (*p == '"' || *p == '\'')
        {
            char quote = *p;
            p++;
            char *start = p;
            while (*p && *p != quote)
            {
                p++;
            }
            size_t len = p - start;
            tok->text = malloc(len + 1);
            memcpy(tok->text, start, len);
            tok->text[len] = '\0';
            tok->is_literal = (quote == '\'');
            if (*p == quote)
            {
                p++;
            }
        }
        else if ((*p == '&' && *(p + 1) == '&') ||
                 (*p == '|' && *(p + 1) == '|') ||
                 (*p == '>' && *(p + 1) == '>'))
        {
            tok->text = malloc(3);
            tok->text[0] = *p;
            tok->text[1] = *(p + 1);
            tok->text[2] = '\0';
            tok->is_literal = 0;
            p += 2;
        }
        else if (*p == ';' || *p == '|' || *p == '<' || *p == '>')
        {
            tok->text = malloc(2);
            tok->text[0] = *p;
            tok->text[1] = '\0';
            tok->is_literal = 0;
            p++;
        }
        else
        {
            char *start = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' &&
                   *p != ';' && *p != '|' && *p != '&' && *p != '<' && *p != '>' &&
                   *p != '"' && *p != '\'')
            {
                p++;
            }
            size_t len = p - start;
            tok->text = malloc(len + 1);
            memcpy(tok->text, start, len);
            tok->text[len] = '\0';
            tok->is_literal = 0;
        }

        tokens[position++] = tok;

        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(ShellToken *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    tokens[position] = NULL;
    return tokens;
}

void lsh_free_tokens(ShellToken **tokens)
{
    if (!tokens)
    {
        return;
    }
    for (int i = 0; tokens[i] != NULL; i++)
    {
        free(tokens[i]->text);
        free(tokens[i]);
    }
    free(tokens);
}

#define LSH_RL_BUFSIZE 1024
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        if (g_interrupted || (c == EOF && errno == EINTR))
        {
            g_interrupted = 0;
            errno = 0;
            clearerr(stdin);
            free(buffer);
            char *empty = malloc(1);
            if (empty)
            {
                empty[0] = '\0';
            }
            return empty;
        }

        if (c == EOF)
        {
            if (position == 0)
            {
                free(buffer);
                return NULL;
            }
            buffer[position] = '\0';
            return buffer;
        }
        else if (c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = (char)c;
        }
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void lsh_loop(void)
{
    char *line;
    ShellToken **tokens;

    while (!g_should_exit)
    {
        lsh_print_prompt();
        line = lsh_read_line();
        if (line == NULL)
        {
            if (isatty(STDIN_FILENO))
            {
                printf("\n");
            }
            break;
        }
        tokens = lsh_split_line(line);
        lsh_execute_line(tokens);

        free(line);
        lsh_free_tokens(tokens);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    lsh_init_signals();
    lsh_loop();
    return g_last_exit_status;
}
