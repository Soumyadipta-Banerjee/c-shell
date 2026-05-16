# c-shell

A minimalist Unix shell implementation written in C, exploring the core concepts of process management, system calls, and the POSIX API.

## Features

- **Process Management**: Uses `fork()`, `execvp()`, and `waitpid()` for child process lifecycle management.
- **Built-in Commands**: Custom implementations of `cd`, `help`, and `exit`.
- **Command Parsing**: Dynamic string tokenization and memory management for handling user input.
- **Error Handling**: Robust error reporting using `perror` and system-level checks.

## Technical Highlights

- **POSIX API**: Leverages industry-standard Unix interfaces for portable system programming.
- **Memory Safety**: Careful management of dynamic buffers using `malloc` and `realloc`.
- **Process Synchronization**: Correct use of wait status macros (`WIFEXITED`, `WIFSIGNALED`) for reliable process tracking.

## Getting Started

### Prerequisites

- A C compiler (GCC or Clang)
- Make (optional, but recommended)

### Build and Run

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/Soumyadipta-Banerjee/c-shell.git
    cd c-shell
    ```

2.  **Build using Makefile**:
    ```bash
    make
    ```

3.  **Run the shell**:
    ```bash
    ./c-shell
    ```

Alternatively, you can run directly with `make run`.

## License

This project is open-source and available under the [MIT License](LICENSE).
