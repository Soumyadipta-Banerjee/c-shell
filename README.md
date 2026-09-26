# c-shell

A feature-rich Unix shell implementation written in C, exploring process management, inter-process communication (IPC), system calls, and the POSIX API.

## Features

- **Process Management**: Uses `fork()`, `execvp()`, and `waitpid()` for child process lifecycle management.
- **Dynamic Colorful Prompt**: Displays `user@hostname:path$` with ANSI styling and tilde (`~`) shortening for the home directory.
- **Built-in Commands**:
  - `cd [dir]`: Changes directory (defaults to `$HOME` if no path or `~` is given).
  - `pwd`: Prints the current working directory.
  - `help`: Displays built-ins and feature summary.
  - `exit [code]`: Terminates the shell session with an optional exit code.
- **Command Chaining**:
  - `;`: Sequential execution (`cmd1 ; cmd2 ; cmd3`).
  - `&&`: Conditional AND execution — runs the next command only if the previous succeeded (`cmd1 && cmd2`).
  - `||`: Conditional OR execution — runs the next command only if the previous failed (`cmd1 || cmd2`).
- **I/O Redirection**:
  - `< filename`: Redirects standard input (`open` + `dup2`).
  - `> filename`: Redirects standard output (create/truncate).
  - `>> filename`: Appends standard output.
- **Pipelines**: Arbitrary multi-stage piping (`cmd1 | cmd2 | ... | cmdN`) using POSIX `pipe()` and `dup2()`.
- **Expansions**:
  - `$?`: Expands to the exit status code of the most recently executed command.
  - `$VAR`: Expands environment variables (e.g. `$USER`, `$HOME`, `$PATH`).
- **Quoted Arguments**: Supports single (`'...'`) and double (`"..."`) quotes for arguments with spaces, with single quotes suppressing expansion.
- **Signal Handling**: Protected interactive prompt against `Ctrl+C` (`SIGINT`) and `Ctrl+Z` (`SIGTSTP`); correctly forwards interruption signals to foreground child processes.
- **Robust Terminal Handling**: Clean `EOF` / `Ctrl+D` exit and non-interactive pipe detection via `isatty()`.

## Project Structure

```text
c-shell/
├── include/
│   ├── builtins.h     # Built-in declarations and dispatch table
│   ├── execute.h      # Process execution, pipelines, and command chaining
│   ├── parser.h       # Tokenization, quoting, line reading, and expansions
│   └── signals.h      # Signal handlers (SIGINT, SIGTSTP)
├── src/
│   ├── builtins.c     # Implementations of cd, pwd, help, exit
│   ├── execute.c      # Execution engine, I/O redirection, and pipelines
│   ├── main.c         # Shell loop entrypoint
│   ├── parser.c       # Tokenizer, dynamic prompt, and variable expansion
│   └── signals.c      # Signal setup and protection
├── tests/
│   └── test_shell.sh  # Automated test suite (30 test cases)
├── .github/
│   └── workflows/
│       └── ci.yml     # Automated CI pipeline
├── Makefile           # Build and test rules
└── README.md
```

## Getting Started

### Prerequisites

- A C compiler (GCC or Clang)
- Make
- Bash (for running the automated test suite)

### Build and Run

1. **Clone the repository**:
   ```bash
   git clone https://github.com/Soumyadipta-Banerjee/c-shell.git
   cd c-shell
   ```

2. **Build using Makefile**:
   ```bash
   make
   ```

3. **Run the shell**:
   ```bash
   ./c-shell
   ```

   Alternatively, build and run directly with:
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

## Usage Examples

```bash
# Quoted arguments with spaces
echo "Hello from Soumya's C Shell!"

# Built-in commands
pwd
cd ..
pwd

# Command Chaining
echo "Step 1" ; echo "Step 2"
make && ./c-shell
cat nonexistent.txt || echo "File not found"

# Expansions
false ; echo $?       # Prints 1
echo "Welcome $USER"  # Expands environment variable
echo '$USER'          # Preserved literally without expansion

# I/O Redirection
echo "Hello World" > output.txt
cat < output.txt
echo "Appended Line" >> output.txt

# Multi-stage Pipelines
cat Makefile | grep TARGET | wc -l

# Exit with code
exit 0
```

## Automated Testing

The project includes an automated test suite ([tests/test_shell.sh](tests/test_shell.sh)) containing **30 test cases** covering:
- Built-in commands (`pwd`, `cd`, `help`, `exit [code]`)
- External command execution
- Quoting behavior (single and double quotes with whitespace preservation)
- Command chaining (`&&`, `||`, `;`) and short-circuit evaluation
- Variable expansion (`$VAR`, `$?`) and literal preservation
- I/O redirection (`<`, `>`, `>>`, combined input/output)
- Multi-stage pipelines and stream filtering
- Signal handling (`SIGINT` shell survival and child interruption)
- Error handling and syntax validation

Run tests anytime with:
```bash
make test
```

## License

This project is open-source and available under the [MIT License](LICENSE).