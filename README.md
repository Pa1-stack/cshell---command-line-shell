# cshell

`cshell` is a small Unix-style command-line shell written in C. It demonstrates process creation, command execution, pipelines, input/output redirection, background jobs, and terminal job control.

This project is intended for learning and experimentation with Linux system calls and shell implementation concepts.

## Features

- Execute external programs using `execvp`
- Built-in commands: `cd`, `pwd`, `exit`, `jobs`, `fg`, `bg`, and `export`
- Pipelines using `|`
- Input redirection using `<`
- Output redirection using `>`
- Append output redirection using `>>`
- Background processes using `&`
- Basic job tracking and job control
- Foreground and background process groups
- `Ctrl+C` and `Ctrl+Z` handling for foreground jobs

## Requirements

- Linux, WSL 2, or another POSIX-compatible environment
- GCC or another C compiler
- Standard C build tools

On Ubuntu or WSL, install the compiler with:

```bash
sudo apt update
sudo apt install build-essential
```

## Build

Clone the repository and enter its directory:

```bash
git clone https://github.com/USERNAME/REPOSITORY.git
cd REPOSITORY
```

Compile the shell:

```bash
gcc -Wall -Wextra -o cshell main.c builtins.c exec.c jobs.c parser.c
```

Run it:

```bash
./cshell
```

On WSL, a Windows project directory can also be accessed through `/mnt/c`. For example:

```bash
cd "/mnt/c/Users/your-windows-user/Desktop/command-line project"
gcc -Wall -Wextra -o cshell main.c builtins.c exec.c jobs.c parser.c
./cshell
```

## Built-in Commands

| Command  | Description                                                                     | Example             |
| -------- | ------------------------------------------------------------------------------- | ------------------- |
| `cd`     | Change the current directory. With no argument, changes to `$HOME`.             | `cd /tmp`           |
| `pwd`    | Print the current directory.                                                    | `pwd`               |
| `exit`   | Exit the shell. An optional numeric status can be supplied.                     | `exit 0`            |
| `jobs`   | List tracked background and stopped jobs.                                       | `jobs`              |
| `fg`     | Continue a job in the foreground. Accepts a job ID or process-group ID.         | `fg 1`              |
| `bg`     | Continue a stopped job in the background. Accepts a job ID or process-group ID. | `bg 1`              |
| `export` | Set an environment variable for commands started by the shell.                  | `export NAME=value` |

## Examples

Run an external command:

```text
cshell:/path/to/project$ ls
```

Use a pipeline:

```text
cshell:/path/to/project$ ls | grep ".c"
```

Redirect command output to a file:

```text
cshell:/path/to/project$ ls > files.txt
cshell:/path/to/project$ echo "another line" >> files.txt
```

Read command input from a file:

```text
cshell:/path/to/project$ sort < names.txt
```

Run a command in the background:

```text
cshell:/path/to/project$ sleep 30 &
[bg] pgid 1234
cshell:/path/to/project$ jobs
```

Stop and resume a foreground process:

```text
Ctrl+Z
bg 1
fg 1
```

## Project Structure

| File         | Purpose                                                               |
| ------------ | --------------------------------------------------------------------- |
| `main.c`     | Shell startup, prompt, input loop, and signal setup                   |
| `shell.h`    | Shared data structures, constants, and function declarations          |
| `parser.c`   | Parses commands, pipelines, redirection, and background execution     |
| `exec.c`     | Creates processes, connects pipes, redirects files, and runs commands |
| `builtins.c` | Implements shell built-in commands                                    |
| `jobs.c`     | Tracks jobs and handles child-process status changes                  |

## Current Limitations

The parser is intentionally small and whitespace-based. It currently does not support:

- Quoted arguments, such as `echo "hello world"`
- Wildcard expansion, such as `*.c`
- Environment-variable expansion, such as `echo $HOME`
- Command chaining with `&&`, `||`, or `;`
- Command history
- A built-in `help` command
- Advanced job-spec syntax such as `%1`

## Learning Goals

This project provides a compact example of:

- `fork`, `execvp`, and `waitpid`
- Unix pipes and file descriptors
- Process groups and terminal ownership
- Signal handling with `SIGCHLD`, `SIGINT`, and `SIGTSTP`
- The difference between built-ins that run inside the shell and external commands that run in child processes
