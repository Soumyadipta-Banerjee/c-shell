# c-shell

A feature-rich Unix shell implementation written in C, exploring process management, inter-process communication (IPC), system calls, and the POSIX API.

## Features

- **Process Management**: Uses `fork()`, `execvp()`, and `waitpid()` for child process lifecycle management.
- **Dynamic Colorful Prompt**: Displays `user@hostname:path$` with ANSI styling and tilde (`~`) shortening for the home directory.
- **Built-in Commands**:
  - `cd [dir]`: Changes directory (defaults to `$HOME` if no path or `~` is given).
  - `pwd`: Prints the current working directory.
  - `help`: Displays built-ins and feature summary.
  - `exit`: Terminates the shell session.
- **I/O Redirection**:
  - `< filename`: Redirects standard input (`open` + `dup2`).
  - `> filename`: Redirects standard output (create/truncate).
  - `>> filename`: Appends standard output.
- **Pipelines**: Arbitrary multi-stage piping (`cmd1 | cmd2 | ... | cmdN`) using POSIX `pipe()` and `dup2()`.
- **Quoted Arguments**: Supports single (`'...'`) and double (`"..."`) quotes for arguments containing spaces.
- **Signal Handling**: Protected interactive prompt against `Ctrl+C` (`SIGINT`) and `Ctrl+Z` (`SIGTSTP`); correctly forwards interruption signals to foreground child processes.
- **Robust Terminal Handling**: Clean `EOF` / `Ctrl+D` exit and non-interactive pipe detection via `isatty()`.

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

# I/O Redirection
echo "Hello World" > output.txt
cat < output.txt
echo "Appended Line" >> output.txt

# Multi-stage Pipelines
cat Makefile | grep TARGET | wc -l

# Exit
exit
```

## Automated Testing

The project includes an automated test suite ([tests/test_shell.sh](tests/test_shell.sh)) covering:
- Built-in commands (`pwd`, `cd`, `help`, `exit`)
- External command execution
- Quoting behavior (single and double quotes with whitespace preservation)
- I/O redirection (`<`, `>`, `>>`, combined input/output)
- Multi-stage pipelines and stream filtering
- Error handling and syntax validation

Run tests anytime with:
```bash
make test
```

## License

This project is open-source and available under the [MIT License](LICENSE).