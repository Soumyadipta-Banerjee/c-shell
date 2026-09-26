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

## Phase 5: Interactive Line Editing & Autocompletion (Linenoise)

### Objectives
Provide full interactive terminal ergonomics without escape-code corruption.

### Requirements & Specifications
1. **Linenoise Integration**:
   - Embed lightweight, single-file line editor library.
   - Support:
     - Left/Right arrow cursor movement.
     - Up/Down history traversal.
     - Home/End key support.
2. **Tab Autocompletion**:
   - File path completion: crawl current directory and complete subpaths.
   - Command completion: complete built-ins and executables in `$PATH`.

---

## Phase 6: Proactive Safety Shield & Command Aliases

### Objectives
Protect users from catastrophic accidental commands and provide user-defined aliases.

### Requirements & Specifications
1. **Accidental Deletion Guard (`safemode`)**:
   - Check if command is `rm` with flags containing `-r` or `-f`:
     - If targeting `/`, `/*`, `*`, or current working directory:
       Prompt with confirmation dialog:
       `⚠️  [SAFETY GUARD] Destructive command detected! Type 'yes' to proceed: `
2. **Command Aliases (`alias` & `unalias`)**:
   - `alias name='command'`: Stores mapping in alias table.
   - Transparently substitute matching tokens during command expansion.

---

## 🧪 Testing & Verification Strategy

Every phase must maintain:
* **Zero Compiler Warnings**: `-Wall -Wextra -pedantic -std=c99 -O2`.
* **Zero Memory Leaks**: Verified using AddressSanitizer (`-fsanitize=address`).
* **Continuous Integration**: GitHub Actions CI workflow running `make && make test` on all pull requests and pushes.
