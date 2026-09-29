# Apex Shell (`apex-shell`)

[![CI](https://github.com/Soumyadipta-Banerjee/apex-shell/actions/workflows/ci.yml/badge.svg)](https://github.com/Soumyadipta-Banerjee/apex-shell/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Standards: C99](https://img.shields.io/badge/C-99-informational.svg)](Makefile)

A high-performance, portfolio-grade Unix shell written in C99 exploring POSIX system calls, process lifecycles, memory safety, dynamic Git prompt integration, algorithmic intelligence, scripting execution, and native systems observability.

---

## Key Features

- **Fish-Style Ghost Text Autosuggestions**:
  - Real-time inline history suggestions rendered dynamically in faint grey ahead of the cursor without altering the line buffer.
  - Transparent cursor alignment preserving 1:1 editing position.
  - Instantly accept suggestions via `Right Arrow` or `End` (when at end of line), or `Ctrl+F` / `Ctrl+E` anywhere on the line.
- **Extended POSIX File Descriptor Redirections**:
  - `2> file`: Redirect standard error to overwrite file (`command 2> errors.log`).
  - `2>> file`: Append standard error to file (`command 2>> errors.log`).
  - `&> file`: Redirect both standard output and standard error to file (`command &> all.log`).
  - `2>&1` & `1>&2`: Duplicate output file descriptors for seamless error stream merging.
- **Built-in Integer Arithmetic Expansion (`$(( expression ))`)**:
  - High-performance recursive-descent integer math evaluation engine.
  - Supports arithmetic (`+`, `-`, `*`, `/`, `%`), relational (`<`, `<=`, `>`, `>=`), equality (`==`, `!=`), logical (`&&`, `||`), unary (`+`, `-`, `!`, `~`), and arbitrary parenthesized groupings.
  - Automatic shell variable resolution (e.g., `VAR=5; echo $(( VAR * 2 + 1 ))` $\to$ `11`).
  - Built-in division-by-zero and modulo-by-zero error detection.
- **Subshell Command Substitution (`$(cmd)` & `` `cmd` ``)**:
  - Subshell execution with anonymous pipe stdout capturing and trailing newline stripping.
  - Seamlessly handles arbitrary nested pipelines, chaining, and redirections (`echo $(cat file | grep pattern | wc -l)`).
  - Preserves command substitution within double quotes (`"Count: $(ls | wc -l)"`) while preserving literal syntax inside single quotes.
- **Live Terminal Syntax Highlighting**:
  - Real-time ANSI token coloring rendered dynamically inside raw `termios` editing mode.
  - Green for valid built-ins and executables, Red for unrecognized commands, Yellow for command options/flags (`-la`), Cyan for quoted strings and arithmetic expressions, and Magenta for control operators (`|`, `&&`, `;`, `>`, `>>`, `2>`, `&>`).
- **Interactive Reverse History Search (`Ctrl+R`)**:
  - `(reverse-i-search)'<query>': <match>` prompt.
  - Incremental substring search with backward cycling across previous commands on repeated `Ctrl+R`.
  - Enter accepts and executes the match, while Esc or `Ctrl+G` cancels and restores original line buffer.
- **Directory Stack & Frecency Navigation (`pushd`, `popd`, `dirs`, `z`)**:
  - LIFO directory stack: `pushd <dir>` pushes current working directory and changes to target; `popd` pops top directory and jumps back; `dirs` displays current stack.
  - Intelligent frecency jumping: `z <pattern>` dynamically ranks directories by visit frequency and recency for instant navigation without typing full paths.
- **POSIX Terminal Job Control & Suspension**:
  - Full process group orchestration using `setpgid()` and foreground terminal delegation via `tcsetpgrp()`.
  - Foreground job suspension via `Ctrl+Z` (`SIGTSTP`), capturing `WIFSTOPPED` and registering jobs as `Stopped`.
  - `jobs`: Lists active background and suspended tasks with state (`Running` vs `Stopped`) and `+`/`-` indicators.
  - `fg [job_id]`: Brings a background or suspended job to the foreground, sending `SIGCONT` if stopped and waiting for execution.
  - `bg [job_id]`: Resumes a suspended job in the background via `SIGCONT`.
  - `kill [-signal] pid | %job_id`: Signals jobs by logical `%id` or process `pid` with numeric and named signal support.
- **Startup Configuration & Sourcing (`source`, `.`, and `~/.apexrc`)**:
  - `source <file>` / `. <file>`: Reads and executes commands directly in the current shell session, dynamically exporting variables and setting aliases.
  - Automatic profile initialization: seamlessly loads `~/.apexrc` on interactive startup if present.
- **Interactive Line Editing & Tab Autocompletion**:
  - Direct raw terminal control (`termios`) with zero escape sequence corruption.
  - Left / Right arrow cursor repositioning, Home (`Ctrl+A`), End (`Ctrl+E`), Backspace, and Delete.
  - Up / Down arrow history browsing cycling through session commands.
  - Tab (`\t`) autocompletion for built-in commands and filesystem paths (directories complete with `/`).
  - Screen clear (`Ctrl+L`), interrupt (`Ctrl+C`), and EOF (`Ctrl+D`) support.
- **Proactive Safety Shield (`safemode`)**:
  - Real-time command interception preventing catastrophic accidental deletions (`rm -rf /`, `rm -rf /*`, `rm -rf ~`, `rm -rf *`, etc.).
  - Interactive mode prompts for user confirmation `[y/N]` before proceeding; scripted or non-interactive execution immediately blocks execution with exit status 1.
  - Built-in `safemode [on|off|status]` to inspect or toggle guardrails at runtime.
- **User-Defined Command Aliases (`alias` & `unalias`)**:
  - `alias name='command'`: Create custom shortcuts with transparent argument passthrough.
  - `alias`: Prints all active aliases in clean `alias name='command'` format.
  - `unalias name` / `unalias -a`: Remove individual aliases or clear the entire alias table.
  - Built-in recursion guard prevents cyclic alias loops.
- **Process Management**: Robust lifecycle orchestration using `fork()`, `execvp()`, and `waitpid()`.
- **Scripting & Non-Interactive Execution**:
  - `apex-shell -c "commands"`: Executes inline one-liners with command chaining, redirections, and pipelines, exiting with the exact status code.
  - `apex-shell script.apex`: Reads and executes shell scripts line by line, gracefully ignoring comments starting with `#`.
- **Persistent Command History**:
  - `history`: Lists recorded commands with line numbers.
  - `history N`: Prints the last `N` recorded commands.
  - `history -c`: Clears in-memory and disk history.
  - Automatically loads and persists up to 1,000 commands to `~/.apex_history`.
- **Algorithmic Intelligence ("Did You Mean?" Suggestions)**:
  - High-performance Damerau-Levenshtein distance algorithm with transposition, deletion, insertion, and substitution handling.
  - Automatically triggered when `execvp` fails with `ENOENT`. Scans built-in commands and binaries in `$PATH` to suggest the closest match (`gti` $\to$ `git`, `pwdd` $\to$ `pwd`, `clea` $\to$ `clear`, `sl` $\to$ `ls`).
  - Conforms to POSIX convention returning exit status 127 for unknown commands.
- **Native Systems Observability & Telemetry**:
  - `time <cmd>`: Command execution profiler intercepting single commands and multi-stage pipelines. Captures wall-clock time (`clock_gettime`), user & system CPU time via kernel `getrusage(RUSAGE_CHILDREN)`, peak resident memory (RSS), minor/major page faults, and voluntary/involuntary context switches without corrupting stdout streams.
  - `sysinfo`: High-speed, zero-dependency kernel dashboard inspecting CPU cores, aggregate load averages (`/proc/loadavg`), RAM & Swap utilization (`/proc/meminfo`), uptime (`/proc/uptime`), and shell process statistics.
- **Dynamic Git-Aware Prompt**:
  - Displays `user@hostname:path (git:branch)$` with ANSI styling and tilde (`~`) home shortening.
  - Native zero-subprocess Git branch discovery parsing `.git/HEAD` directly without incurring fork overhead.
- **Built-in Commands**:
  - `cd [dir]`: Changes directory (supports `~`, no args defaults to `$HOME`).
  - `pwd`: Prints the current working directory.
  - `pushd <dir>`: Pushes current directory onto the stack and navigates to target.
  - `popd`: Pops the top directory off the stack and changes to it.
  - `dirs`: Displays the current directory stack.
  - `z <pattern>`: Smart frecency directory jump to most frequent/recent matching directory.
  - `export KEY=VALUE`: Sets environment variables for the shell and child processes.
  - `unset KEY`: Unsets environment variables.
  - `env`: Lists active environment variables.
  - `fg [%id]`: Brings a background or stopped job to the foreground terminal.
  - `bg [%id]`: Resumes a stopped job in the background.
  - `kill [-signal] pid | %id`: Sends a signal to a process or job.
  - `jobs`: Lists active background and stopped jobs with state indicators.
  - `source <file>` / `. <file>`: Executes commands from file in current shell environment.
  - `alias [name='val']`: Sets or lists user-defined command aliases.
  - `unalias [name|-a]`: Removes specific or all command aliases.
  - `safemode [on|off|status]`: Configures or inspects the accidental deletion shield.
  - `sysinfo`: Renders the live system observability dashboard.
  - `history`: Displays or clears command history.
  - `help`: Interactive summary of commands, syntax, and features.
  - `exit [code]`: Exits shell with specified or last status code.
- **Background Jobs & Reaping**:
  - `&`: Launches processes or pipelines asynchronously in the background (`sleep 5 &`).
  - Synchronous, non-blocking zombie reaping using `waitpid(..., WNOHANG)`.
  - Notification when background tasks finish (`[1]+ Done`).
- **Command Chaining & Operators**:
  - `;`: Sequential execution (`cmd1 ; cmd2 ; cmd3`).
  - `&&`: Conditional AND execution — runs subsequent command only if previous succeeded (`cmd1 && cmd2`).
  - `||`: Conditional OR execution — runs subsequent command only if previous failed (`cmd1 || cmd2`).
- **I/O Redirection**:
  - `< file`: Standard input redirection (`open` + `dup2`).
  - `> file`: Standard output redirection (create / truncate).
  - `>> file`: Standard output append mode.
  - Combined `< in.txt > out.txt` redirection.
- **Pipelines**: Arbitrary multi-stage piping (`cmd1 | cmd2 | ... | cmdN`) using POSIX `pipe()` and `dup2()`.
- **Expansions**:
  - `$?`: Exit code of previous foreground command.
  - `$$`: Process ID of shell instance.
  - `$VAR` & `${VAR}`: Environment variable expansion inside tokens and strings.
  - `~`: Tilde expansion to user's `$HOME` (`~/projects`).
- **Quoting & Literal Preservation**: Single quotes (`'...'`) preserve exact literals without expansion; double quotes (`"..."`) preserve internal spaces and perform variable expansion.
- **Signal Handling**: Protected interactive prompt against `Ctrl+C` (`SIGINT`) and `Ctrl+Z` (`SIGTSTP`), correctly delegating signals to foreground jobs while shielding background tasks.
- **Terminal & Stream Robustness**: Handles `Ctrl+D` (`EOF`), empty lines, and non-interactive pipes gracefully.

---

## Project Structure

```text
apex-shell/
├── include/
│   ├── alias.h        # Command alias table and expansion declarations
│   ├── arithmetic.h   # Recursive-descent integer arithmetic evaluator ($(( ... )))
│   ├── builtins.h     # Built-in declarations and dispatch table
│   ├── completion.h   # Tab autocompletion for built-ins and directory paths
│   ├── execute.h      # Process execution, pipelines, and command chaining
│   ├── expander.h     # Macro, variable ($VAR), command $(), and arithmetic bridge
│   ├── fuzzy.h        # Damerau-Levenshtein distance & command suggestions
│   ├── highlight.h    # Real-time ANSI syntax token coloring
│   ├── history.h      # Command history and persistent serialization
│   ├── jobs.h         # Background job tracking and non-blocking reaping
│   ├── linereader.h   # Raw termios line editing, ghost text, keystroke dispatch
│   ├── parser.h       # Lexer, quoting state machine, and token lifecycle
│   ├── prompt.h       # Git branch detection and dynamic prompt rendering
│   ├── redirection.h  # Extended FD redirections (<, >, 2>, 2>>, &>, 2>&1)
│   ├── safety.h       # Proactive safety shield against destructive commands
│   ├── signals.h      # Signal handlers (SIGINT, SIGTSTP)
│   └── telemetry.h    # Observability: time profiler and sysinfo dashboard
├── src/
│   ├── alias.c        # Alias dictionary and recursive-safe substitution
│   ├── arithmetic.c   # Integer arithmetic parser, precedence, and logic
│   ├── builtins.c     # Implementations of cd, pwd, export, unset, env, alias, etc.
│   ├── completion.c   # Filesystem and built-in tab autocompletion engine
│   ├── execute.c      # Execution engine, pipelines, and command chaining
│   ├── expander.c     # Variable, command substitution, and math expansion
│   ├── fuzzy.c        # Typo correction and PATH binary candidate discovery
│   ├── highlight.c    # Real-time ANSI terminal syntax highlighter
│   ├── history.c      # In-memory history buffer and file persistence (~/.apex_history)
│   ├── jobs.c         # Job list management and zombie process cleanup
│   ├── linereader.c   # Raw termios engine, ghost text suggestions, cursor motion
│   ├── main.c         # REPL loop, script execution (.apex), and -c execution
│   ├── parser.c       # Pure lexer, tokenizer, and token memory management
│   ├── prompt.c       # Git branch discovery and dynamic ANSI prompt
│   ├── redirection.c  # Extended file descriptor redirection engine
│   ├── safety.c       # Destructive command interception and safemode engine
│   ├── signals.c      # Signal setup and prompt protection
│   └── telemetry.c    # getrusage profiling and /proc system metrics parser
├── tests/
│   ├── run_tests.sh       # Master test orchestrator
│   ├── helpers/           # Reusable assertion library (test_framework.sh)
│   ├── fixtures/          # Shared test scripts (.apex) and profile templates
│   ├── integration/       # Domain-driven integration suites (86 tests)
│   │   ├── test_builtins.sh
│   │   ├── test_pipelines.sh
│   │   ├── test_substitutions.sh
│   │   ├── test_jobs.sh
│   │   ├── test_safety.sh
│   │   ├── test_observability.sh
│   │   ├── test_fuzzy.sh
│   │   └── test_scripting.sh
│   └── unit/              # Algorithmic C unit tests (51 assertions)
│       ├── test_fuzzy.c
│       ├── test_alias.c
│       └── test_arithmetic.c
├── .github/
│   └── workflows/
│       └── ci.yml     # Automated CI pipeline
├── Makefile           # Strict build rules (-Wall -Wextra -pedantic -std=c99 -O2)
├── ARCHITECTURE.md    # System design, memory ownership, and extensibility guide
├── ROADMAP.md         # Long-term feature specifications and engineering milestones
└── README.md
```

---

## Documentation

- **[System Architecture (ARCHITECTURE.md)](ARCHITECTURE.md)**: In-depth breakdown of component boundaries, execution dataflow, memory ownership contracts, and extensibility patterns.
- **[Long-term Product Roadmap (ROADMAP.md)](ROADMAP.md)**: Detailed phase-by-phase specifications for systems observability, algorithmic intelligence, scripting, and safety shields.

---

## Getting Started

### Prerequisites

- A C99 compiler (`gcc` or `clang`)
- `make`
- `bash` (for automated test suite)

### Build and Run

1. **Clone the repository**:
   ```bash
   git clone https://github.com/Soumyadipta-Banerjee/apex-shell.git
   cd apex-shell
   ```

2. **Build with strict warnings**:
   ```bash
   make
   ```

3. **Launch Apex Shell**:
   ```bash
   ./apex-shell
   ```
   Or launch directly via make:
   ```bash
   make run
   ```

4. **Run a script or inline command**:
   ```bash
   ./apex-shell script.apex
   ./apex-shell -c "sysinfo && time sleep 0.1"
   ```

5. **Run tests**:
   ```bash
   make test       # Run all 8 integration suites (78 tests)
   make test-fast  # Ultra-fast runner skipping sleep tests (< 1.5s)
   make test-unit  # Algorithmic C unit tests (25 assertions)
   make test-all   # Complete test suite: integration + unit (103 assertions)
   ```

6. **Clean build artifacts**:
   ```bash
   make clean
   ```

---

## Usage Examples

```bash
# Git-aware prompt appears automatically in Git repositories:
# soumya@archlinux:~/apex-shell (git:main)$

# Non-interactive inline execution
apex-shell -c "echo Running && sysinfo"

# Persistent Command History
history
history 5
history -c

# Algorithmic Intelligence: catches typos and suggests fixes
$ gti status
apex-shell: 'gti' command not found
Did you mean: 'git'?

$ pwdd
apex-shell: 'pwdd' command not found
Did you mean: 'pwd'?

# Systems Observability: inspect kernel resources
sysinfo

# Execution Telemetry: profile any command or pipeline
time sleep 0.2
time cat Makefile | grep TARGET | wc -l

# User-Defined Command Aliases
alias ll='ls -la'
alias gco='git checkout'
alias
ll /tmp
unalias ll

# Proactive Safety Shield
safemode status
safemode on
rm -rf /               # Intercepted and blocked: Dangerous deletion prevented!
safemode off

# POSIX Job Control
sleep 60 &             # Starts job in background ([1] <pid>)
jobs                   # Lists active and stopped jobs
kill -STOP %1          # Suspends background job (or Ctrl+Z in foreground)
jobs                   # [1]+ Stopped sleep 60
bg %1                  # Resumes job in background ([1]+ sleep 60 &)
fg %1                  # Brings job back to foreground
kill -9 %1             # Force kills job

# Startup Profiles & Sourcing
source ~/.apexrc       # Dynamically loads variables and aliases
. custom_profile.apex  # Dot notation synonym for source

# Subshell Command Substitution
echo "Date is $(date +%Y-%m-%d)"
COUNT=$(ls | wc -l)
echo "Total files: $COUNT"
echo `uname -s -r`

# Arithmetic Expansion ($(( ... )))
echo $(( 10 + 20 * 2 ))        # Prints 50
echo $(( (100 - 20) / 4 ))     # Prints 20
X=15; Y=3
echo "Result: $(( X % Y == 0 ))" # Prints 1

# Extended File Descriptor Redirections
ls nonexistent 2> error.log    # Redirects stderr to file
echo "Both streams" &> all.log # Redirects stdout & stderr
cat input.txt > out.log 2>&1   # Merges stderr into stdout

# Directory Stack Navigation
pushd /tmp             # Pushes current directory and switches to /tmp
dirs                   # Displays: /tmp /home/soumya/apex-shell
popd                   # Returns back to /home/soumya/apex-shell

# Smart Frecency Directory Jump
z apex                 # Instantly jumps to highest-ranked match for 'apex'

# Interactive Line Editing & Syntax Highlighting (in terminal)
# Real-time ANSI syntax colors: Green for valid commands, Red for invalid,
# Yellow for flags, Cyan for strings, Magenta for operators.
# [Ctrl+R] triggers reverse history search: (reverse-i-search)'query': match
# [Tab] completes built-in commands and file paths
# [Up/Down] arrows browse previous command history
# [Left/Right] arrows navigate input buffer
# [Ctrl+L] clears screen, [Ctrl+C] discards current line

# Environment variables
export PROJECT_NAME=apex-shell
echo "Welcome to $PROJECT_NAME!"
echo ${PROJECT_NAME}_v1
env | grep PROJECT_NAME
unset PROJECT_NAME

# Background jobs
sleep 5 &
jobs

# Command Chaining
echo "Step 1" ; echo "Step 2"
make && ./apex-shell
cat nonexistent.txt || echo "File not found"

# Expansions
false ; echo $?       # Prints 1
echo "Shell PID: $$"  # Prints PID
echo ~/projects       # Expands to /home/user/projects

# I/O Redirection
echo "Hello Apex" > output.txt
cat < output.txt
echo "Appended text" >> output.txt

# Multi-stage Pipelines
cat Makefile | grep TARGET | wc -l

# Exit with code
exit 0
```

---

## Automated Testing & Verification

Apex Shell features a modular, two-tier test architecture combining native **C unit tests** with domain-driven **black-box integration suites** managed by `tests/run_tests.sh`:

### Test Organization
- **C Unit Tests (`tests/unit/`)**: 51 assertions testing pure algorithms directly in C:
  - `test_fuzzy.c`: Damerau-Levenshtein distance calculation, transpositions, substitutions, and suggestion ranking (17 assertions).
  - `test_alias.c`: In-memory alias dictionary, insertion, lookup, overwriting, unsetting, and teardown (8 assertions).
  - `test_arithmetic.c`: Recursive-descent math parser, precedence, unary/binary/relational ops, variables, division-by-zero checks (26 assertions).
- **Integration Test Suites (`tests/integration/`)**: 86 test cases partitioned across 8 dedicated domains:
  - `test_builtins.sh`: Built-in commands (`pwd`, `cd`, `export`, `unset`, `env`, `dirs`, `pushd`, `popd`, `z`, `help`, `exit`).
  - `test_pipelines.sh`: Pipelines, standard & extended I/O redirections (`<`, `>`, `>>`, `2>`, `2>>`, `&>`, `2>&1`), chaining operators (`;`, `&&`, `||`).
  - `test_substitutions.sh`: Subshell command substitutions (`$(cmd)` & `` `cmd` ``), arithmetic expansion (`$(( ... ))`), quoting (`''`, `""`), and variable expansions (`$VAR`, `${VAR}`, `$?`, `~`).
  - `test_jobs.sh`: Background execution (`&`), job tracking (`jobs`), process signaling (`kill`), foreground/background control (`fg`, `bg`), and signal protection (`SIGINT`, `SIGTSTP`).
  - `test_safety.sh`: Proactive safety shield (`safemode`), dangerous deletion prevention (`rm -rf /`), and command aliases (`alias`, `unalias`).
  - `test_observability.sh`: Command execution profiler (`time`) and kernel telemetry dashboard (`sysinfo`).
  - `test_fuzzy.sh`: Typo correction suggestions and POSIX exit code 127 handling.
  - `test_scripting.sh`: Non-interactive one-liners (`-c`), script execution (`.apex`), comment parsing, and environment sourcing (`source`, `.`).

### Test Execution Commands
```bash
# Run all integration suites (86 tests)
make test

# Ultra-fast runner skipping sleep tests (< 1.5s)
make test-fast

# Compile and run native C algorithmic unit tests (51 assertions)
make test-unit

# Run specific integration suite (e.g. substitutions, builtins, jobs, safety)
make test-suite SUITE=substitutions

# Run the complete test battery (Integration + Unit = 137 assertions)
make test-all
```

---

## License

This project is open-source and available under the [MIT License](LICENSE).