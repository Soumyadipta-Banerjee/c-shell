# c-shell System Architecture & Engineering Design

This document details the architectural layout, component boundaries, execution flow, memory ownership contracts, and upgradability patterns of `c-shell`. It serves as the canonical design document for future extensions.

---

## 1. High-Level System Architecture

```text
                                 User Input (TTY / Script / Pipe)
                                                │
                                                ▼
                                    ┌───────────────────────┐
                                    │    REPL Controller    │  [src/main.c]
                                    │      (lsh_loop)       │
                                    └───────────┬───────────┘
                                                │  (Raw line string)
                                                ▼
                                    ┌───────────────────────┐
                                    │  Lexer & Tokenizer    │  [src/parser.c]
                                    │   (lsh_split_line)    │
                                    └───────────┬───────────┘
                                                │  (Array of ShellToken*)
                                                ▼
                                    ┌───────────────────────┐
                                    │   Chaining Executor   │  [src/execute.c]
                                    │  (lsh_execute_line)   │  Handles: ;, &&, ||, &
                                    └───────────┬───────────┘
                                                │  (Per-command arguments)
                                                ▼
                         ┌─────────────────────────────────────────────┐
                         │              Dispatch Routing               │
                         └──────────────┬───────────────┬──────────────┘
                                        │               │
                        Is Built-in?    │               │  Is External Command?
                                        ▼               ▼
                        ┌───────────────────┐   ┌───────────────────────┐
                        │ Built-in Registry │   │   Pipeline / Launch   │  [src/execute.c]
                        │ [src/builtins.c]  │   │  Engine (fork/exec)   │
                        └─────────┬─────────┘   └───────────┬───────────┘
                                  │                         │
                                  ▼                         ▼
                        ┌───────────────────┐   ┌───────────────────────┐
                        │ Execution Status  │   │  Job Control & Reap   │  [src/jobs.c]
                        │ (Exit Code / $?)  │   │ (waitpid / WNOHANG)   │
                        └───────────────────┘   └───────────────────────┘
```

---

## 2. Component Boundaries & Responsibilities

| Subsystem | Header | Implementation | Primary Responsibilities |
| :--- | :--- | :--- | :--- |
| **Main / Lifecycle** | `include/parser.h`, `include/execute.h` | `src/main.c` | Program entrypoint, signals init, jobs init, REPL loop, exit teardown. |
| **Parser & Lexer** | `include/parser.h` | `src/parser.c` | Line reading (`getchar`), quoting state machine, tokenization, operator splitting (`;`, `&&`, `||`, `&`, `|`, `<`, `>`), and runtime variable expansion (`$VAR`, `$?`). |
| **Execution Engine** | `include/execute.h` | `src/execute.c` | Chaining flow control (short-circuit logic), argument expansion, file redirection (`dup2`), multi-stage pipeline creation, and process launching. |
| **Built-ins** | `include/builtins.h` | `src/builtins.c` | In-process commands that modify shell state (`cd`, `exit`, `pwd`, `help`, `jobs`, and future `export`, `unset`, `alias`). |
| **Job Control** | `include/jobs.h` | `src/jobs.c` | Background task tracking linked list, non-blocking asynchronous zombie reaping (`waitpid` with `WNOHANG`), and job status formatting. |
| **Telemetry & Observability** | `include/telemetry.h` | `src/telemetry.c` | Process profiling (`getrusage`, `clock_gettime`), execution telemetry interceptor (`time`), and `/proc` system resource dashboard (`sysinfo`). |
| **Algorithmic Intelligence** | `include/fuzzy.h` | `src/fuzzy.c` | Damerau-Levenshtein distance calculation, typo correction, and PATH executable candidate discovery on `ENOENT`. |
| **History & Scripting** | `include/history.h` | `src/history.c`, `src/main.c` | Script file parsing (`.apex`), one-liner execution (`-c`), command history recording, and `~/.apex_history` persistence. |
| **Interactive Line Editor** | `include/linereader.h` | `src/linereader.c` | Raw terminal mode (`termios`), cursor navigation, history traversal, and Tab autocompletion for built-ins and paths. |
| **Command Aliases** | `include/alias.h` | `src/alias.c` | In-memory alias mapping table, recursion-safe token expansion in executor, `alias` and `unalias` built-ins. |
| **Safety Shield** | `include/safety.h` | `src/safety.c` | Proactive interception of destructive commands (`rm -rf /`, `~`), confirmation prompt, and `safemode` control. |
| **Signal Handling** | `include/signals.h` | `src/signals.c` | POSIX `sigaction` registration for `SIGINT` and `SIGTSTP`, shielding the interactive prompt and delegating signals to foreground child processes. |

---

## 3. Data Structures & Memory Ownership Contracts

### 3.1 Token Representation (`ShellToken`)
```c
typedef struct {
    char *text;      // Dynamically allocated string slice
    int is_literal;  // 1 if enclosed in single quotes ('...'), 0 otherwise
} ShellToken;
```
* **Ownership**: `lsh_split_line()` dynamically allocates the array of tokens and each token's inner text.
* **Teardown**: The caller (`lsh_loop`) is strictly responsible for invoking `lsh_free_tokens(tokens)`, which frees each `text`, each `ShellToken`, and the array container.
* **Separation of Expansion**: Expansion occurs at command execution time, not during initial line tokenization. This guarantees that commands chained via `;` or `&&` reflect the updated `$?` and environment variables modified by preceding commands on the same line.

### 3.2 Background Job Table (`Job`)
```c
typedef struct Job {
    int id;              // Logical job identifier (1, 2, 3...)
    pid_t pid;           // Operating system PID (or process group leader)
    char *cmd_name;      // Heap-allocated copy of the original command string
    struct Job *next;    // Singly linked list pointer
} Job;
```
* **Concurrency Safety**: Background jobs are reaped synchronously inside `jobs_reap()` at the start of each REPL cycle and before executing built-ins like `jobs`. This avoids async-signal-safety violations (e.g. calling `printf` or `malloc`/`free` inside a `SIGCHLD` handler).
* **ID Compaction**: When the job list empties out completely, `next_job_id` resets back to 1.

### 3.3 Alias Mapping Table (`Alias`)
```c
typedef struct Alias {
    char *name;          // Alias trigger token (e.g., "ll", "gco")
    char *value;         // Expanded command string (e.g., "ls -la", "git checkout")
    struct Alias *next;  // Singly linked list pointer
} Alias;
```
* **Recursion Guard**: Expansion tracks expansion depth or identity matching to prevent infinite loops when an alias aliases itself (e.g. `alias ls='ls --color=auto'`).
* **Substitution Timing**: Intercepted in `lsh_execute()`: if `args[0]` matches an alias, it is replaced and recursively repacked before built-in or external execution.

### 3.4 Interactive Line Editor & Autocompletion
* **Raw Termios State**: When `isatty(STDIN_FILENO)` is true, the shell configures the terminal into raw mode (`ICANON`, `ECHO`, `ISIG` disabled) with `VMIN=1` and `VTIME=0`.
* **Fallback Guarantee**: In non-interactive contexts (pipes, redirection, scripts), `lsh_read_interactive_line()` falls back to standard `lsh_read_line()` with zero terminal escape overhead.
* **Autocompletion**: Intercepts `\t` (Tab). Scans registered built-in commands and the current working directory via `opendir`/`readdir`. If a unique candidate is found, completes inline; if multiple candidates share a prefix, completes the longest common prefix.

### 3.5 Proactive Safety Shield (`Safety Shield`)
* **Destructive Command Interception**: Intercepts `rm` invocations containing recursive flags (`-r`, `-R`, `--recursive`).
* **Critical Barriers**: Protects `/`, `/*`, `~`, `$HOME`, `.`, `..`, and bare `*`.
* **Interactive vs Automated**: Prompts the user with `[y/N]` confirmation in interactive mode. Under non-interactive mode or automated scripts, execution is blocked with exit code 1 to protect the host machine.
* **Runtime Toggle**: Configurable at runtime via `safemode on`, `safemode off`, or `safemode status`.

---

## 4. Upgradability & Extensibility Guidelines

When adding new capabilities, adhere to the following architectural conventions:

### 4.1 Adding a New Built-in Command
1. **Declare Prototype** in `include/builtins.h`:
   ```c
   int lsh_mycmd(char **args);
   ```
2. **Implement Command** in `src/builtins.c`:
   - Accept `char **args` where `args[0]` is the command name and `args[1..n]` are arguments.
   - Return standard exit status: `0` on success, non-zero on error.
3. **Register in Dispatch Table** in `src/builtins.c`:
   - Add `"mycmd"` to `builtin_str[]`.
   - Add `&lsh_mycmd` to `builtin_func[]`.
   - Update `lsh_help()` with user-facing documentation.

### 4.2 Adding Pre-Execution / Post-Execution Hooks (e.g. Profiling / Safety Guards)
* All command execution funnels through `lsh_execute(char **args, int is_bg)` in `src/execute.c`.
* Insert interceptors prior to `fork()`:
  - **Safety Check**: Inspect `args` for dangerous patterns (e.g., recursive deletion of root or wildcard directories) before dispatching.
  - **Benchmarking Setup**: Capture `clock_gettime(CLOCK_MONOTONIC)` and initial `getrusage(RUSAGE_CHILDREN)` before `waitpid()`, then calculate the delta after child termination.

### 4.3 Signal Safety Model
* **Interactive Prompt State**: `SIGINT` handler sets `g_interrupted = 1` and writes `\n` using async-safe `write()`. `SA_RESTART` is disabled so blocking `getchar()` calls fail with `EINTR`.
* **Foreground Child State**: Parent temporarily ignores `SIGINT` and `SIGTSTP`, while the child resets handlers to `SIG_DFL`.
* **Background Child State**: Background children explicitly set `SIGINT` and `SIGQUIT` to `SIG_IGN` upon forking, ensuring terminal interrupts (`Ctrl+C`) never abort running background jobs.

---

## 5. Build System & Compilation Standards

* **Standard**: C99 (`-std=c99`) with POSIX.1-2008 definitions (`-D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE`).
* **Warning Rigor**: Clean compilation under `-Wall -Wextra -pedantic -O2`.
* **Include Conventions**: Quotes for project headers (`#include "execute.h"`), brackets for system libraries (`#include <sys/wait.h>`).
* **Verification**: All architectural changes must pass the automated test suite (`make test`) and GitHub Actions CI.
