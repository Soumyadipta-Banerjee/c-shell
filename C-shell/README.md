# C Shell Project

A simple UNIX shell implementation written in C, created as a learning project.

## Features

* Reads commands from standard input.
* Parses command lines into arguments.
* Executes external commands using `fork` and `execvp`.
* Includes built-in commands: `cd`, `help`, and `exit`.

## How to Build and Run

1.  **Compile the source code:**
    ```bash
    gcc -o my_shell *.c
    ```

2.  **Run the shell:**
    ```bash
    ./my_shell
    ```

## Usage Example
echo "Hello from my shell!"
Hello from my shell!
ls -l
total 16
-rwxr-xr-x 1 user user 8424 Oct 6 12:40 my_shell
-rw-r--r-- 1 user user  891 Oct 6 12:30 shell.c
exit