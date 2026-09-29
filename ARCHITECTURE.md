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
| **Parser & Lexer** | `include/parser.h` | `src/parser.c` | Line reading (`getchar`), quoting state machine, tokenization, operator splitting (`;`, `&&`, `||`, `&`, `|`, `<`, `>`), and token lifecycle. |
| **Expansion Engine** | `include/expander.h` | `src/expander.c` | Runtime variable expansion (`$VAR`, `${VAR}`, `$?`, `$$`), subshell command capture (`$(cmd)`, `` `cmd` ``), arithmetic bridge, and tildes (`~`). |
| **Prompt & Git** | `include/prompt.h` | `src/prompt.c` | Dynamic PS1 prompt engine (`\u`, `\h`, `\w`, `\W`, `\t`, `\d`, `\g`, `\$`, `\e`, `\n`), zero-subprocess Git branch discovery (`.git/HEAD`). |
| **Execution Engine** | `include/execute.h` | `src/execute.c` | Chaining flow control (short-circuit logic), argument expansion, wildcard glob expansion, multi-stage pipeline creation, and process launching. |
| **Wildcard Globbing** | `include/globber.h` | `src/globber.c` | POSIX `glob()` wildcard pathname pattern expansion (`*`, `?`, `[...]`), literal quote preservation, and argument vector reconstitution. |
| **Redirection & Heredoc** | `include/redirection.h` | `src/redirection.c` | Extended FD redirections (`<`, `>`, `>>`, `2>`, `2>>`, `&>`, `2>&1`, `1>&2`), herestrings (`<<<`), heredoc multi-line capture (`<< DELIM`), and anonymous pipe streaming. |
| **Built-ins** | `include/builtins.h` | `src/builtins.c` | In-process commands that modify shell state (`cd`, `exit`, `pwd`, `help`, `jobs`, `export`, `unset`, `pushd`, `popd`, `dirs`, `z`, `source`). |
| **Job Control** | `include/jobs.h` | `src/jobs.c` | Background task tracking linked list, non-blocking asynchronous zombie reaping (`waitpid` with `WNOHANG`), and job status formatting. |
| **Telemetry & Observability** | `include/telemetry.h` | `src/telemetry.c` | Process profiling (`getrusage`, `clock_gettime`), execution telemetry interceptor (`time`), and `/proc` system resource dashboard (`sysinfo`). |
| **Algorithmic Intelligence** | `include/fuzzy.h` | `src/fuzzy.c` | Damerau-Levenshtein distance calculation, typo correction, and PATH executable candidate discovery on `ENOENT`. |
| **History & Scripting** | `include/history.h` | `src/history.c`, `src/main.c` | Script file parsing (`.apex`), one-liner execution (`-c`), command history recording, and `~/.apex_history` persistence. |
| **Interactive Line Editor** | `include/linereader.h` | `src/linereader.c` | Raw terminal mode (`termios`), cursor navigation, ghost text suggestions, history browsing, and `Ctrl+R` reverse search. |
| **Syntax Highlighter** | `include/highlight.h` | `src/highlight.c` | Real-time ANSI token coloring (commands, options, strings, variables, control operators, and arithmetic expressions). |
| **Tab Autocompletion** | `include/completion.h` | `src/completion.c` | Filesystem traversal (`opendir`/`readdir`), built-in command completion, and longest common prefix calculation. |
| **Command Aliases** | `include/alias.h` | `src/alias.c` | In-memory alias mapping table, recursion-safe token expansion in executor, `alias` and `unalias` built-ins. |
| **Safety Shield** | `include/safety.h` | `src/safety.c` | Proactive interception of destructive commands (`rm -rf /`, `~`), confirmation prompt, and `safemode` control. |
| **Arithmetic Engine** | `include/arithmetic.h` | `src/arithmetic.c` | Recursive-descent integer math evaluator (`$(( ... ))`), operator precedence, comparisons, logic, and division safety. |
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

### 3.2 POSIX Job Table & State Machine (`Job`)
```c
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED
} JobStatus;

typedef struct Job {
    int id;              // Logical job identifier (1, 2, 3...)
    pid_t pid;           // Operating system PID (or process group leader)
    char *cmd_name;      // Heap-allocated copy of the original command string
    JobStatus status;    // Execution state: JOB_RUNNING or JOB_STOPPED
    struct Job *next;    // Singly linked list pointer
} Job;
```
* **Process Group Management**: When launching jobs (`lsh_launch` and `lsh_execute_pipeline`), child processes are assigned their own process group via `setpgid()`. The shell delegates foreground terminal control using `tcsetpgrp(STDIN_FILENO, pgroup)` and reclaims it when the child terminates or stops.
* **Job Suspension (`Ctrl+Z`)**: When a foreground child receives `SIGTSTP`, `waitpid(pid, &status, WUNTRACED)` detects `WIFSTOPPED(status)`. The child is immediately enqueued into the job list with status `JOB_STOPPED` and reported to the terminal.
* **Job Resumption**:
  - `fg [job_id]`: Brings a stopped or background job to the foreground, sends `SIGCONT` if stopped, hands off the terminal via `tcsetpgrp`, and waits synchronously.
  - `bg [job_id]`: Sends `SIGCONT` to a stopped job, transitioning it to `JOB_RUNNING` in the background.
* **Concurrency Safety**: Background and stopped jobs are harvested non-blockingly via `waitpid(..., WNOHANG | WUNTRACED | WCONTINUED)` at each REPL iteration.
* **ID Compaction**: When the job list empties out completely, `next_job_id` resets back to 1.

### 3.3 Alias Mapping Table (`Alias`)
```c
typedef struct Alias {
    char *name;          // Alias trigger token (e.g., "ll", "gco")
    char *value;         // Expanded command string (e.g., "ls -la", "git checkout")
    struct Alias *next;  // Singly linked list pointer
} Alias;
```

### 3.4 Arithmetic Engine & Extended Redirections
* **Recursive Descent AST-less Evaluator**: `evaluate_arithmetic_expression()` processes mathematical expressions with standard C operator precedence without requiring heap-allocated ASTs:
  - Level 1: Logical OR (`||`)
  - Level 2: Logical AND (`&&`)
  - Level 3: Equality / Inequality (`==`, `!=`)
  - Level 4: Relational (`<`, `<=`, `>`, `>=`)
  - Level 5: Additive (`+`, `-`)
  - Level 6: Multiplicative (`*`, `/`, `%`)
  - Level 7: Unary (`+`, `-`, `!`, `~`)
  - Level 8: Primary (integers, variables `$VAR` or `VAR`, parenthesized sub-expressions `(expr)`).
* **Extended File Descriptors**: `handle_redirection()` processes redirection tokens sequentially in the child process using `dup2()`:
  - `2>` and `2>>`: `open(file, O_WRONLY | O_CREAT | (TRUNC/APPEND), 0644)` followed by `dup2(fd, STDERR_FILENO)`.
  - `&>`: Redirects both `STDOUT_FILENO` and `STDERR_FILENO` to the same file.
  - `2>&1`: `dup2(STDOUT_FILENO, STDERR_FILENO)`.
  - `1>&2`: `dup2(STDERR_FILENO, STDOUT_FILENO)`.

### 3.5 Interactive Line Editor & Autocompletion
* **Raw Termios State**: When `isatty(STDIN_FILENO)` is true, the shell configures the terminal into raw mode (`ICANON`, `ECHO`, `ISIG` disabled) with `VMIN=1` and `VTIME=0`.
* **Fish-Style Ghost Text**: Evaluates history prefix matches on every keystroke and projects suggestions in faint grey (`\033[90m`), maintaining transparent cursor positioning. Accepts suggestions via `Right Arrow`, `End`, `Ctrl+F`, or `Ctrl+E`.
* **Fallback Guarantee**: In non-interactive contexts (pipes, redirection, scripts), `lsh_read_interactive_line()` falls back to standard `lsh_read_line()` with zero terminal escape overhead.
* **Autocompletion**: Intercepts `\t` (Tab). Scans registered built-in commands and the current working directory via `opendir`/`readdir`. If a unique candidate is found, completes inline; if multiple candidates share a prefix, completes the longest common prefix.

### 3.6 Proactive Safety Shield (`Safety Shield`)
* **Destructive Command Interception**: Intercepts `rm` invocations containing recursive flags (`-r`, `-R`, `--recursive`).
* **Critical Barriers**: Protects `/`, `/*`, `~`, `$HOME`, `.`, `..`, and bare `*`.
* **Interactive vs Automated**: Prompts the user with `[y/N]` confirmation in interactive mode. Under non-interactive mode or automated scripts, execution is blocked with exit code 1 to protect the host machine.
* **Runtime Toggle**: Configurable at runtime via `safemode on`, `safemode off`, or `safemode status`.

### 3.7 Subshell Command Substitution (`$(...)` and `` `...` ``)
* **Anonymous Pipe IPC**: `capture_command_output()` creates an anonymous pipe via `pipe()`, forks a subshell child, redirects child `stdout` to the pipe write end, and executes the inner command line using `lsh_split_line()` and `lsh_execute_line()`.
* **Output Processing**: The parent process drains the read end of the pipe into a dynamically resizing heap buffer, awaits child termination (`waitpid`), strips trailing `\r`/`\n` characters per POSIX specification, and splices the resulting string into the expanding token stream.

### 3.8 Directory Stack & Frecency State (`pushd`, `popd`, `dirs`, `z`)
* **Directory Stack**: Static LIFO array (`s_dir_stack[64]`) storing heap-allocated directory paths. `pushd` saves current directory and changes to target; `popd` returns to previous stack entry.
* **Frecency Matrix**: Maintains directory visits and dynamic weights in `s_frecency[128]`. Whenever directory changes succeed (`cd`, `pushd`, `popd`), scores increase. The `z` command performs ranked substring matching to execute instant jumps.

### 3.9 Pathname Wildcard Globbing Engine (`globber`)
* **POSIX `glob()` Integration**: Prior to command execution, `expand_tokens_with_glob()` inspects token arguments using `has_glob_meta()` for meta-characters `*`, `?`, and `[...]`.
* **Strict Quoting Preservation**: Single-quoted tokens (`is_literal == 1`) bypass expansion to preserve literal wildcard symbols.
* **Fallback & Reconstitution**: Unmatched patterns default to literal strings (`GLOB_NOCHECK`). Matching entries are sorted alphabetically by libc and reconstituted into an expanded `char **` argument vector, freed via `free_glob_args()`.

### 3.10 Heredocs & Herestrings Anonymous Pipe Streaming
* **Heredoc Capture (`<< DELIM`)**: `resolve_heredocs()` scans user input lines for `<< DELIM` tokens. In interactive or batch mode, subsequent lines are buffered until the standalone delimiter appears, transformed into an escaped herestring `<<< "content"`.
* **Herestring Anonymous Pipe (`<<<`)**: `handle_redirection()` detects `<<<` and writes the target string payload directly into an anonymous kernel pipe (`pipe()`), connecting the read end to `STDIN_FILENO` with `dup2()`.

### 3.11 Configurable Dynamic PS1 Prompt Engine
* **Format Specifier Lexer**: `format_ps1()` dynamically inspects `$PS1`, parsing escape sequences (`\u`, `\h`, `\H`, `\w`, `\W`, `\t`, `\d`, `\g`, `\$`, `\e`, `\n`, `\\`).
* **Git Repository Telemetry**: `\g` natively checks `.git/HEAD` and git status without spawning child processes.
* **Fallback**: When `$PS1` is unset, the prompt seamlessly falls back to the default Git-aware ANSI colored prompt format.

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

---

## 6. Modular Test Architecture & Verification Harness

The test subsystem follows a two-tier testing pyramid combining in-memory algorithmic C unit tests with domain-driven black-box integration suites:

```text
tests/
├── run_tests.sh               # Master test runner & summary orchestrator
├── helpers/
│   └── test_framework.sh      # Reusable assertion library (assert_equals, assert_contains, assert_status)
├── fixtures/                  # Shared test inputs and script templates (.apex, configs)
│   ├── sample.apex            # Script execution test fixture
│   └── profile.apex           # Startup configuration sourcing fixture
├── integration/               # Black-box shell execution suites partitioned by domain
│   ├── test_builtins.sh       # pwd, cd, export, unset, env, pushd, popd, dirs, z (15 tests)
│   ├── test_pipelines.sh      # |, <, >, >>, 2>, 2>>, &>, 2>&1, <<<, << DELIM, ;, &&, || (22 tests)
│   ├── test_substitutions.sh  # $(), ``, $(( )), *, ?, [..], '', "", $?, $VAR, ${VAR}, ~ (18 tests)
│   ├── test_jobs.sh           # &, jobs, fg, bg, kill, SIGINT, SIGTSTP (11 tests)
│   ├── test_safety.sh         # safemode, dangerous deletion interception, aliases (7 tests)
│   ├── test_observability.sh  # time profiler and sysinfo dashboard (5 tests)
│   ├── test_fuzzy.sh          # Typo correction, Damerau-Levenshtein, exit 127 (5 tests)
│   └── test_scripting.sh      # -c one-liners, .apex files, source/. (11 tests)
└── unit/                      # Direct C unit tests for internal algorithms
    ├── test_fuzzy.c           # Damerau-Levenshtein dynamic programming matrix edge cases (17 tests)
    ├── test_alias.c           # In-memory alias dictionary, lookup, overwrite, cleanup (8 tests)
    ├── test_arithmetic.c      # Arithmetic engine, precedence, variables, error states (26 tests)
    ├── test_glob.c            # Wildcard globbing, meta detection, quotes, fallback (19 tests)
    └── test_prompt.c          # PS1 format specifiers, user, host, git branch, cwd, colors (24 tests)
```

### 6.1 Reusable Test Assertion Harness (`test_framework.sh`)
* **Sandbox Isolation**: Each integration suite generates an isolated temporary directory via `mktemp -d` and binds a POSIX `trap ... EXIT` to guarantee complete cleanup of scratch files on test termination.
* **Unified Assertions**: Encapsulates comparison primitives (`exact`, `contains`, `not_contains`, `exit_only`, `non_zero`) with ANSI colored pass/fail reporting and failure diff inspection.

### 6.2 Developer Ergonomics & Feedback Loops
* **Instant Feedback (`make test-fast`)**: Skips sleep-based process management tests to validate parser, built-in, pipeline, and syntax logic in under 1.5 seconds.
* **Domain Targeting (`make test-suite SUITE=<name>`)**: Executes a single integration suite for focused feature debugging (e.g. `make test-suite SUITE=substitutions`).
* **C Unit Testing (`make test-unit`)**: Compiles and verifies algorithmic core components directly in native C in under 30ms (94 unit assertions).
* **Full Battery (`make test-all`)**: Runs all 8 integration suites (94 tests) and all 5 C unit suites (94 assertions) in sequence (188 total assertions).

