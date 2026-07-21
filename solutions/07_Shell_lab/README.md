# Shell Lab

The purpose of this assignment is to become more familiar with the concepts of process control and signalling. Students will do this by writing a simple Unix shell program that supports job control.

# General Overview of Unix Shell

A shell is an interactive command-line interpreter that runs programs on behalf of the user. A shell repeatedly prints a prompt, waits for a command line on stdin, and then carries out some action, as directed by
the contents of the command line.
The command line is a sequence of ASCII text words delimited by whitespace. The first word in the
command line is either the name of a built-in command or the pathname of an executable file. The remaining
words are command-line arguments. If the first word is a built-in command, the shell immediately executes
the command in the current process. Otherwise, the word is assumed to be the pathname of an executable
program. In this case, the shell forks a child process, then loads and runs the program in the context of the
child. The child processes created as a result of interpreting a single command line are known collectively
as a job. In general, a job can consist of multiple child processes connected by Unix pipes.
If the command line ends with an ampersand ”&”, then the job runs in the background, which means that
the shell does not wait for the job to terminate before printing the prompt and awaiting the next command
line. Otherwise, the job runs in the foreground, which means that the shell waits for the job to terminate
before awaiting the next command line. Thus, at any point in time, at most one job can be running in the
foreground. However, an arbitrary number of jobs can run in the background.


# What the shell has to do

Looking at the tsh.c (tiny shell) file, you will see that it contains a functional skeleton of a simple Unix
shell. To help you get started, we have already implemented the less interesting functions. Your assignment
1
is to complete the remaining empty functions listed below. As a sanity check for you, we’ve listed the
approximate number of lines of code for each of these functions in our reference solution (which includes
lots of comments).
- eval: Main routine that parses and interprets the command line. [70 lines]
- builtin cmd: Recognizes and interprets the built-in commands: quit, fg, bg, and jobs. [25
lines]
- do bgfg: Implements the bg and fg built-in commands. [50 lines]
- waitfg: Waits for a foreground job to complete. [20 lines]
- sigchld handler: Catches SIGCHILD signals. 80 lines]
- sigint handler: Catches SIGINT (ctrl-c) signals. [15 lines]
•-sigtstp handler: Catches SIGTSTP (ctrl-z) signals. [15 lines]

## Implementation

[`tsh.c`](../../Labs/shlab-handout/shlab-handout/tsh.c) — the full tiny shell:

- [`eval`](../../Labs/shlab-handout/shlab-handout/tsh.c#L170) — the read/fork/exec loop
- [`parseline`](../../Labs/shlab-handout/shlab-handout/tsh.c#L213) — split the command line into argv, detect `&`
- [`builtin_cmd`](../../Labs/shlab-handout/shlab-handout/tsh.c#L275) — dispatch `quit`/`jobs`/`bg`/`fg`
- [`do_bgfg`](../../Labs/shlab-handout/shlab-handout/tsh.c#L300) — the `bg` and `fg` built-ins
- [`waitfg`](../../Labs/shlab-handout/shlab-handout/tsh.c#L389) — block until the foreground job is done
- [`sigchld_handler`](../../Labs/shlab-handout/shlab-handout/tsh.c#L411) — reap terminated/stopped children
- [`sigint_handler`](../../Labs/shlab-handout/shlab-handout/tsh.c#L459) — forward Ctrl-C to the foreground group
- [`sigtstp_handler`](../../Labs/shlab-handout/shlab-handout/tsh.c#L487) — forward Ctrl-Z to the foreground group

# The concurrency problems (the actual point of the lab)

This is the part of the textbook (CS:APP Chapter 8, Exceptional Control Flow) that the lab
forces you to actually implement instead of just read:

1. addjob / reap race
2. Busy-wait in `waitfg`
3. Async-signal-safety
4. Reaping all children

# Testing

The lab ships 16 trace files and a reference shell (`tshref`). The shell is "correct" when its
output matches the reference on every trace. I verified this by diffing my output against the
reference for all 16:

```
for i in $(seq -w 1 16); do
  echo "=== trace$i ==="
  diff <(./sdriver.pl -t trace$i.txt -s ./tsh    -a "-p") \
       <(./sdriver.pl -t trace$i.txt -s ./tshref -a "-p")
done
```

Every trace passes. The only differences that show up are the PID numbers themselves
(e.g. `(71318)` vs `(71319)`), which are assigned by the OS and necessarily differ between two
separate runs
