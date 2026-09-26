# Apex Shell (`apex-shell`)

[![CI](https://github.com/Soumyadipta-Banerjee/apex-shell/actions/workflows/ci.yml/badge.svg)](https://github.com/Soumyadipta-Banerjee/apex-shell/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Standards: C99](https://img.shields.io/badge/C-99-informational.svg)](Makefile)

A high-performance, portfolio-grade Unix shell written in C99 exploring POSIX system calls, process lifecycles, memory safety, dynamic Git prompt integration, algorithmic intelligence, and native systems observability.

---

## Key Features

- **Process Management**: Robust lifecycle orchestration using `fork()`, `execvp()`, and `waitpid()`.
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
  - `jobs`: Lists active background jobs with status and command details.
  - `sysinfo`: Renders the live system observability dashboard.
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
│   ├── builtins.h     # Built-in declarations and dispatch table
│   ├── execute.h      # Process execution, pipelines, and command chaining
│   ├── fuzzy.h        # Damerau-Levenshtein distance & command suggestions
│   ├── jobs.h         # Background job tracking and non-blocking reaping
│   ├── parser.h       # Tokenization, quoting, prompt, and expansions
│   ├── signals.h      # Signal handlers (SIGINT, SIGTSTP)
│   └── telemetry.h    # Observability: time profiler and sysinfo dashboard
├── src/
│   ├── builtins.c     # Implementations of cd, pwd, export, unset, env, jobs, sysinfo, help, exit
│   ├── execute.c      # Execution engine, I/O redirection, and pipelines
│   ├── fuzzy.c        # Typo correction and PATH binary candidate discovery
│   ├── jobs.c         # Job list management and zombie process cleanup
│   ├── main.c         # Interactive REPL shell loop entrypoint
│   ├── parser.c       # Tokenizer, zero-overhead git discovery, and variable expansion
│   ├── signals.c      # Signal setup and prompt protection
│   └── telemetry.c    # getrusage profiling and /proc system metrics parser
├── tests/
│   └── test_shell.sh  # Automated test suite (48 test cases)
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

4. **Run the automated test suite**:
   ```bash
   make test
   ```

5. **Clean build artifacts**:
   ```bash
   make clean
   ```

---

## Usage Examples

```bash
# Git-aware prompt appears automatically in Git repositories:
# soumya@archlinux:~/apex-shell (git:main)$

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

The project includes an automated test suite ([tests/test_shell.sh](tests/test_shell.sh)) containing **48 test cases** covering:
- Algorithmic Intelligence (Damerau-Levenshtein fuzzy matching, built-in and PATH typo suggestions, exit 127)
- Systems Observability & Telemetry (`time` command rusage profiler, status preservation, `sysinfo` dashboard)
- Built-in commands (`pwd`, `cd`, `export`, `unset`, `env`, `jobs`, `sysinfo`, `help`, `exit [code]`)
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