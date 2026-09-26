# Apex Shell (`apex-shell`)

[![CI](https://github.com/Soumyadipta-Banerjee/apex-shell/actions/workflows/ci.yml/badge.svg)](https://github.com/Soumyadipta-Banerjee/apex-shell/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Standards: C99](https://img.shields.io/badge/C-99-informational.svg)](Makefile)

A high-performance, portfolio-grade Unix shell written in C99 exploring POSIX system calls, process lifecycles, memory safety, dynamic Git prompt integration, algorithmic intelligence, scripting execution, and native systems observability.

---

## Key Features

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
  - `export KEY=VALUE`: Sets environment variables for the shell and child processes.
  - `unset KEY`: Unsets environment variables.
  - `env`: Lists active environment variables.
  - `alias [name='val']`: Sets or lists user-defined command aliases.
  - `unalias [name|-a]`: Removes specific or all command aliases.
  - `safemode [on|off|status]`: Configures or inspects the accidental deletion shield.
  - `jobs`: Lists active background jobs with status and command details.
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
│   ├── builtins.h     # Built-in declarations and dispatch table
│   ├── execute.h      # Process execution, pipelines, and command chaining
│   ├── fuzzy.h        # Damerau-Levenshtein distance & command suggestions
│   ├── history.h      # Command history and persistent serialization
│   ├── jobs.h         # Background job tracking and non-blocking reaping
│   ├── linereader.h   # Raw termios line editing and tab autocompletion
│   ├── parser.h       # Tokenization, quoting, prompt, and expansions
│   ├── safety.h       # Proactive safety shield against destructive commands
│   ├── signals.h      # Signal handlers (SIGINT, SIGTSTP)
│   └── telemetry.h    # Observability: time profiler and sysinfo dashboard
├── src/
│   ├── alias.c        # Alias dictionary and recursive-safe substitution
│   ├── builtins.c     # Implementations of cd, pwd, export, unset, env, alias, unalias, safemode, jobs, sysinfo, history, help, exit
│   ├── execute.c      # Execution engine, I/O redirection, pipelines, and hooks
│   ├── fuzzy.c        # Typo correction and PATH binary candidate discovery
│   ├── history.c      # In-memory history buffer and file persistence (~/.apex_history)
│   ├── jobs.c         # Job list management and zombie process cleanup
│   ├── linereader.c   # Raw termios engine, cursor motion, and tab autocompletion
│   ├── main.c         # REPL loop, script execution (.apex), and -c execution
│   ├── parser.c       # Tokenizer, zero-overhead git discovery, and variable expansion
│   ├── safety.c       # Destructive command interception and safemode engine
│   ├── signals.c      # Signal setup and prompt protection
│   └── telemetry.c    # getrusage profiling and /proc system metrics parser
├── tests/
│   └── test_shell.sh  # Automated test suite (61 test cases)
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

5. **Run the automated test suite**:
   ```bash
   make test
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

# Interactive Line Editing & Autocompletion (in terminal)
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

## Automated Testing

The project includes an automated test suite ([tests/test_shell.sh](tests/test_shell.sh)) containing **61 test cases** covering:
- Interactive Line Editing & Autocompletion (`termios` raw mode, buffer traversal, tab completion)
- Command Aliases (`alias`, `unalias`, recursive prevention, argument passing)
- Proactive Safety Shield (`safemode`, catastrophic `rm` command blocking, confirmation checks)
- Scripting & One-Liners (`-c` execution, `.apex` file execution, `#` comment ignoring, exit codes)
- Persistent History (`history` built-in, numerical limits, serialization)
- Algorithmic Intelligence (Damerau-Levenshtein fuzzy matching, built-in and PATH typo suggestions, exit 127)
- Systems Observability & Telemetry (`time` command rusage profiler, status preservation, `sysinfo` dashboard)
- Built-in commands (`pwd`, `cd`, `export`, `unset`, `env`, `alias`, `unalias`, `safemode`, `jobs`, `sysinfo`, `history`, `help`, `exit [code]`)
- Background processes (`&`), job tracking, and asynchronous zombie reaping
- External command execution
- Quoting behavior (single and double quotes with whitespace preservation)
- Command chaining (`&&`, `||`, `;`) and short-circuit evaluation
- Variable expansion (`$VAR`, `${VAR}`, `$?`, `$$`, `~`) and literal preservation
- I/O redirection (`<`, `>`, `>>`, combined input/output)
- Multi-stage pipelines and stream filtering
- Signal handling (`SIGINT` shell survival and child interruption)
- Error handling and syntax validation

Run tests anytime with:
```bash
make test
```

---

## License

This project is open-source and available under the [MIT License](LICENSE).