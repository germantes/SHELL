# Linux Job Control Shell

A UNIX-like shell implementation with job control support, developed as part of the Operating Systems course at the University of Malaga (UMA).

Based on concepts from "Operating System Concepts Essentials" by Silberschatz et al.

## Features

- **Job Control**: Manage background and foreground processes
- **Built-in Commands**: `cd`, `exit`, `jobs`, `fg`, `bg`
- **I/O Redirection**: Support for `<` (input) and `>` (output) redirection
- **Signal Handling**: Proper management of SIGCHLD, SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, and SIGTTOU
- **Process Groups**: Automatic process group management for job control

## Built-in Commands

| Command | Description |
|---------|-------------|
| `cd <dir>` | Change current working directory |
| `jobs` | List all background/suspended jobs |
| `fg [n]` | Bring job `n` to the foreground (default: most recent) |
| `bg [n]` | Resume job `n` in the background (default: most recent) |
| `exit` | Exit the shell |

## Usage

```bash
# Compile
gcc shell.c job_control.c -o shell

# Run
./shell

# Exit
# Press Ctrl+D or type 'exit'
```

## Project Structure

| File | Description |
|------|-------------|
| `shell.c` | Main shell loop, built-in commands, and signal handler |
| `job_control.c` | Job list management, command parsing, and signal utilities |
| `job_control.h` | Type definitions, function prototypes, and macros |

## Shell Notation

- Use `&` at the end of a command to run it in the background
- Use `Ctrl+Z` to suspend a foreground process
- Use `Ctrl+C` to terminate a foreground process (when not intercepted)

## Authors

Operating Systems Course - Dept. de Arquitectura de Computadores, UMA
