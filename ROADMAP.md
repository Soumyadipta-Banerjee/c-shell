# c-shell Product & Engineering Roadmap

This document outlines the master roadmap for evolving `c-shell` into **Apex Shell** — a modern, feature-rich Unix shell with native systems observability, developer ergonomics, and safety guards.

---

## 🗺️ Master Roadmap Overview

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        Apex Shell Master Plan                          │
├────────────┬───────────────────────────────────────────────────────────┤
│ Phase 1    │ Environment & Git Integration (Prompt + export/unset/env) │
├────────────┼───────────────────────────────────────────────────────────┤
│ Phase 2    │ Systems Observability & Profiling (time + sysinfo)        │
├────────────┼───────────────────────────────────────────────────────────┤
│ Phase 3    │ Algorithmic Intelligence (Levenshtein "Did You Mean?")     │
├────────────┼───────────────────────────────────────────────────────────┤
│ Phase 4    │ Scripting Engine & History Persistence                    │
├────────────┼───────────────────────────────────────────────────────────┤
│ Phase 5    │ Interactive Line Editor & Tab Autocompletion (Linenoise)  │
├────────────┼───────────────────────────────────────────────────────────┤
│ Phase 6    │ Proactive Safety Shield & Command Aliases                 │
├────────────┼───────────────────────────────────────────────────────────┤
│ Phase 7    │ POSIX Job Control (fg/bg/kill/Ctrl+Z) & Startup Profile   │
├────────────┼───────────────────────────────────────────────────────────┤
│ Phase 8    │ Command Substitution $(), Syntax Colors, Ctrl+R, pushd/z  │
└────────────┴───────────────────────────────────────────────────────────┘
```

---

## Phase 1: Environment Management & Git-Aware Prompt

### Objectives
Complete the environment lifecycle so variables can be set, unset, and inspected dynamically, and enhance the prompt with live Git status.

### Requirements & Specifications
1. **Environment Built-ins**:
   - `export KEY=VALUE`: Parses `KEY` and `VALUE` using `strchr` and sets it via `setenv(KEY, VALUE, 1)`.
   - `export KEY`: Exports existing variable or marks it.
   - `unset KEY`: Removes variable via `unsetenv(KEY)`.
   - `env`: Iterates over `extern char **environ` and prints all environment pairs.
2. **Tilde Expansion**:
   - In `expand_token()`, expand any token beginning with `~/` to `$HOME/`.
3. **Git-Aware Prompt**:
   - Helper function `char *get_git_branch(void)`:
     - Checks if `.git/HEAD` exists in current directory or parent directories.
     - Reads `.git/HEAD`: if it begins with `ref: refs/heads/<branch>`, extracts `<branch>`.
     - Checks if working tree has untracked/modified files.
   - Render prompt format:
     ```text
     user@host:path (main ✗)$ 
     ```
     - Green for user/host, Blue for directory, Cyan for branch, Red `✗` for dirty, Green `✓` for clean.

### Implementation Checklist
- [x] Implement `lsh_export`, `lsh_unset`, `lsh_env` built-ins.
- [x] Add Git repository detection and branch reader in `src/parser.c`.
- [x] Add tilde expansion and braced variable expansion `${VAR}` in `src/parser.c`.
- [x] Add automated tests for `export`, `unset`, `env`, and `~/` expansion in `tests/test_shell.sh`.

---

## Phase 2: Systems Observability & Telemetry

### Objectives
Turn `apex-shell` into a first-class tool for systems programmers by providing hardware, kernel, and process resource telemetry.

### Requirements & Specifications
1. **Command Execution Profiler (`time` Built-in)**:
   - Measures:
     - **Wall Clock Time**: `clock_gettime(CLOCK_MONOTONIC)` delta.
     - **CPU Times**: `ru_utime` (user) and `ru_stime` (system) via `getrusage(RUSAGE_CHILDREN, ...)`.
     - **Peak Memory**: `ru_maxrss` (maximum resident set size in KB/MB).
     - **Page Faults**: `ru_minflt` (minor, no I/O) and `ru_majflt` (major, disk I/O).
     - **Context Switches**: `ru_nvcsw` (voluntary) and `ru_nivcsw` (involuntary).
   - Display format:
     ```text
     ┌─ Execution Telemetry ──────────────────────────────────────────┐
     │ Wall Time: 0.1420s   | CPU: 0.1080s user, 0.0340s sys          │
     │ Peak RAM:  32.40 MB  | Page Faults: 1842 minor, 0 major        │
     │ Context Switches: 42 voluntary, 8 involuntary                  │
     └────────────────────────────────────────────────────────────────┘
     ```
2. **Built-in Kernel Inspector (`sysinfo`)**:
   - Parses Linux `/proc/stat` and `/proc/loadavg` for CPU cores and system load.
   - Parses `/proc/meminfo` for Total, Free, and Available RAM and Swap.
   - Parses `/proc/uptime` for system uptime.
   - Outputs a fast, zero-dependency ASCII dashboard.

### Implementation Checklist
- [x] Create `include/telemetry.h` and `src/telemetry.c`.
- [x] Implement `time` command interceptor in execution engine (`lsh_execute_timed`).
- [x] Implement `sysinfo` built-in command with `/proc` parsing and shell resource telemetry.
- [x] Add tests verifying `time` command output structure, exit code retention, and pipelines (tests 39-43).

---

## Phase 3: Algorithmic Intelligence (Levenshtein "Did You Mean?")

### Objectives
Enhance developer experience by catching typos with algorithmic fuzzy matching.

### Requirements & Specifications
1. **Levenshtein Distance Algorithm**:
   - Classic dynamic programming algorithm computing minimum edit distance (insertions, deletions, substitutions) between strings.
2. **Command Scanner**:
   - When `execvp()` returns `ENOENT` (command not found):
     - Scan list of all shell built-ins.
     - Scan common directories in `$PATH` (`/bin`, `/usr/bin`, `/usr/local/bin`).
     - Find the candidate with the lowest distance $\le 2$.
   - Output suggestion:
     ```text
     c-shell: 'gti' command not found.
     Did you mean: 'git'?
     ```

### Implementation Checklist
- [x] Create `include/fuzzy.h` and `src/fuzzy.c`.
- [x] Implement Damerau-Levenshtein distance algorithm with transposition support.
- [x] Implement candidate finder scanning built-ins and `$PATH` with intelligent tie-breaking.
- [x] Connect into error path when `execvp` fails with `ENOENT` returning exit status 127.
- [x] Add automated tests asserting suggestions on common typos (`gti` -> `git`, `pwdd` -> `pwd`, `clea` -> `clear`, tests 44-48).

---

## Phase 4: Scripting Engine & History Persistence

### Objectives
Enable writing and executing shell scripts, inline `-c` execution, and persistent command history across terminal sessions.

### Requirements & Specifications
1. **Script File Execution**:
   - If argument passed to `./c-shell <script.sh>`:
     - Open file, read lines sequentially, ignore lines starting with `#` (comments), execute lines via `lsh_execute_line`.
     - Exit with the status code of the last command.
2. **One-liner Execution (`-c`)**:
   - `./c-shell -c "cmd1 && cmd2"` runs command string without entering interactive prompt.
3. **Command History**:
   - `history` built-in prints numbered command log.
   - On shell boot: read `~/.cshell_history` into memory.
   - On command entry: append command to history buffer.
   - On shell exit: write updated history to `~/.cshell_history`.

### Implementation Checklist
- [x] Modify `src/main.c` argument handling to detect `-c` and file paths.
- [x] Create `include/history.h` and `src/history.c`.
- [x] Implement `history` built-in, numerical limits (`history N`), clear (`history -c`), and persistent serialization (`~/.apex_history`).
- [x] Add automated tests for script execution, `-c` inline one-liners, and history commands (tests 49-54).

---

## Phase 5: Interactive Line Editing & Autocompletion

### Objectives
Provide full interactive terminal ergonomics without escape-code corruption, powered by raw termios and tab autocompletion.

### Requirements & Specifications
1. **Interactive Line Editor**:
   - Left/Right arrow cursor movement.
   - Up/Down history traversal through recorded session commands.
   - Home/End key support.
   - Backspace, Delete, Ctrl+C, Ctrl+D, and Ctrl+L screen clear.
2. **Tab Autocompletion**:
   - File path completion: crawl current directory and complete subpaths and directories (with trailing `/`).
   - Command completion: complete built-ins and executables.
   - Longest common prefix completion for multiple candidates.

### Implementation Checklist
- [x] Create `include/linereader.h` and `src/linereader.c`.
- [x] Implement raw termios handling, escape sequence parser, and cursor repositioning.
- [x] Implement Tab autocompletion for built-ins and directory paths.
- [x] Integrate cleanly into `src/main.c` with zero impact on non-interactive pipelines.

---

## Phase 6: Proactive Safety Shield & Command Aliases

### Objectives
Protect users from catastrophic accidental commands and provide user-defined aliases.

### Requirements & Specifications
1. **Accidental Deletion Guard (`safemode`)**:
   - Intercept destructive `rm` commands targeting root (`/`, `/*`), home (`~`, `$HOME`), or recursive directories.
   - Prompt with confirmation in interactive mode or block in scripted mode.
   - `safemode [on|off|status]` built-in command.
2. **Command Aliases (`alias` & `unalias`)**:
   - `alias name='command'`: Stores mapping in alias table.
   - Transparently substitute matching tokens during command expansion.
   - `unalias name` / `unalias -a`: Remove aliases.

### Implementation Checklist
- [x] Create `include/alias.h` and `src/alias.c`.
- [x] Create `include/safety.h` and `src/safety.c`.
- [x] Implement alias expansion and safety guard checks in `src/execute.c`.
- [x] Add automated tests for aliases, unalias, and safemode (tests 55-61).

---

## Phase 7: POSIX Job Control & Startup Profile

### Objectives
Turn `apex-shell` into a complete process orchestrator with full POSIX job control (foreground/background switching, interactive job suspension via `Ctrl+Z`, process groups) and startup environment configuration via `~/.apexrc`.

### Requirements & Specifications
1. **Full Job Lifecycle & Suspension**:
   - `JobStatus`: Distinguish between `JOB_RUNNING` and `JOB_STOPPED`.
   - Intercept `WIFSTOPPED(status)` in `waitpid(..., WUNTRACED)` when child receives `SIGTSTP` (`Ctrl+Z`).
   - Add suspended child to job table and output formatted status `[id]+  Stopped  <cmd>`.
   - Non-blocking asynchronous status harvesting with `waitpid(..., WNOHANG | WUNTRACED | WCONTINUED)`.
2. **Foreground & Background Commands (`fg` & `bg`)**:
   - `fg [job_id]`: Brings job to foreground terminal, restores terminal process group via `tcsetpgrp()`, sends `SIGCONT` if stopped, waits for completion, and restores terminal to shell.
   - `bg [job_id]`: Resumes stopped job in background via `SIGCONT` and transitions state to `JOB_RUNNING`.
3. **Signal Sender (`kill`)**:
   - `kill [-signal] pid | %job_id`: Supports targeting jobs by logical `%id` or OS `pid`, supporting numeric (`-9`, `-15`) and symbolic signals (`-KILL`, `-TERM`, `-STOP`, `-CONT`, `-INT`).
4. **Startup Configuration (`source`, `.`, and `~/.apexrc`)**:
   - `source <file>` / `. <file>`: Reads and executes commands line-by-line within the current shell process, allowing variable exports, aliases, and functions to persist.
   - Interactive startup automatically checks and loads `~/.apexrc` if present.

### Implementation Checklist
- [x] Update `include/jobs.h` and `src/jobs.c` with `JobStatus`, stopped job tracking, and `lsh_fg`, `lsh_bg`, `lsh_kill`.
- [x] Implement process group management (`setpgid`) and terminal hand-off (`tcsetpgrp`) in `src/execute.c`.
- [x] Implement `source` and `.` built-ins in `src/builtins.c`.
- [x] Auto-load `~/.apexrc` on interactive startup in `src/main.c`.
- [x] Add automated tests for `source`, `.`, `jobs` status, `kill %id`, `bg`, and `fg` (tests 62-70).

---

## Phase 8: Command Substitution, Live Highlighting & Navigation

### Objectives
Equip `apex-shell` with subshell command substitution (`$(...)` & `` `...` ``), live terminal syntax highlighting, interactive reverse history search (`Ctrl+R`), and directory stack/frecency jumping (`pushd`, `popd`, `dirs`, `z`).

### Requirements & Specifications
1. **Command Substitution**:
   - Subshell fork and anonymous pipe capture for `$(command)` and `` `command` ``.
   - Preserves arbitrary pipelines and multi-command streams inside substitution.
   - Strips trailing newlines and splices stdout directly into expanded tokens.
2. **Live Syntax Highlighting**:
   - Colorizes terminal input line in real-time within raw `termios` engine.
   - Green for valid built-ins/executables, Red for unknown commands.
   - Yellow for flags (`-la`), Cyan for quoted strings, Magenta for operators (`|`, `&&`, `;`, `>`).
3. **Interactive Reverse History Search (`Ctrl+R`)**:
   - `(reverse-i-search)'<query>': <match>` prompt.
   - Incremental substring search cycling backwards on repeated `Ctrl+R`.
   - `Enter` accepts and executes, `Esc`/`Ctrl+G` cancels and restores input.
4. **Directory Stack & Frecency (`pushd`, `popd`, `dirs`, `z`)**:
   - `pushd <dir>`, `popd`, and `dirs` directory stack management.
   - `z <query>` frecency directory jumper automatically learning visited paths.

### Implementation Checklist
- [x] Implement subshell output capture in `src/parser.c` for `$()` and `` `...` ``.
- [x] Implement live syntax highlighting in `src/linereader.c`.
- [x] Implement interactive `Ctrl+R` reverse search loop in `src/linereader.c`.
- [x] Implement `pushd`, `popd`, `dirs`, and `z` in `src/builtins.c`.
- [x] Add automated tests for substitution and directory navigation (tests 71-78).

---

## 🧪 Testing & Verification Strategy

Every phase must maintain:
* **Zero Compiler Warnings**: `-Wall -Wextra -pedantic -std=c99 -O2`.
* **Zero Memory Leaks**: Verified using AddressSanitizer (`-fsanitize=address`).
* **Continuous Integration**: GitHub Actions CI workflow running `make && make test` on all pull requests and pushes.

