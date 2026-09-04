# Tiny Shell with Job Control

A Unix shell in C supporting foreground/background execution, job control via Ctrl-C and
Ctrl-Z, and the `quit`/`jobs`/`bg`/`fg` built-ins. Output matches the reference shell on all
16 trace files.

Source: [`tsh.c`](../../Labs/shlab-handout/shlab-handout/tsh.c)

| function | role |
|---|---|
| [`eval`](../../Labs/shlab-handout/shlab-handout/tsh.c#L167) | parse, fork, exec; add the job and wait or return |
| [`builtin_cmd`](../../Labs/shlab-handout/shlab-handout/tsh.c#L268) | dispatch `quit` / `jobs` / `bg` / `fg` |
| [`do_bgfg`](../../Labs/shlab-handout/shlab-handout/tsh.c#L293) | resume a job by `%jid` or PID, into fg or bg |
| [`waitfg`](../../Labs/shlab-handout/shlab-handout/tsh.c#L378) | block until the foreground job leaves the foreground |
| [`sigchld_handler`](../../Labs/shlab-handout/shlab-handout/tsh.c#L400) | reap terminated and stopped children |
| [`sigint_handler`](../../Labs/shlab-handout/shlab-handout/tsh.c#L448) | forward Ctrl-C to the foreground process group |
| [`sigtstp_handler`](../../Labs/shlab-handout/shlab-handout/tsh.c#L476) | forward Ctrl-Z to the foreground process group |

## Results

The lab ships 16 traces and a reference binary, `tshref`. Correct means the output matches
the reference on every trace:

```sh
for i in $(seq -w 1 16); do
  diff <(./sdriver.pl -t trace$i.txt -s ./tsh    -a "-p") \
       <(./sdriver.pl -t trace$i.txt -s ./tshref -a "-p")
done
```

| trace | what it exercises | |
|---|---|:--:|
| 01-03 | EOF on stdin, `quit`, a foreground job | ✅ |
| 04 | background job with `&` | ✅ |
| 05 | two background jobs, `jobs` listing | ✅ |
| 06 | Ctrl-C kills the foreground job | ✅ |
| 07 | Ctrl-C kills *only* the foreground job, background survives | ✅ |
| 08 | Ctrl-Z stops the foreground job | ✅ |
| 09 | `bg %n` restarts a stopped job in the background | ✅ |
| 10 | `fg %n`, Ctrl-Z, `fg %n` again | ✅ |
| 11 | Ctrl-C reaches the whole process *group* (verified with `ps`) | ✅ |
| 12 | Ctrl-Z reaches the whole process group | ✅ |
| 13 | `fg` restarts an entire stopped group | ✅ |
| 14 | error handling: `./bogus`, `fg`/`bg` with no arg, non-numeric arg, nonexistent job | ✅ |
| 15 | all of the above combined | ✅ |
| 16 | SIGTSTP/SIGINT arriving from *another process*, not the terminal | ✅ |

The only diffs are the PID numbers themselves, which the OS assigns and which necessarily
differ between two separate runs.

Progression, by commit:

| version | change |
|---|---|
| `do_bgfg` | `bg`/`fg` argument parsing |
| `first draft` | `eval`, handlers, job-list wiring |
| `fixed no job found output, double fork, wrong execv arguments, return instead of exit in child` | child-path corrections |
| `added CTRL-C terminated message` | SIGINT reporting |
| `CTRL-Z stopped output` | SIGTSTP reporting |
| `fixed logic bug that checks if FG is occupied` | `fg` guard |
| `Fixed converting pid2jid after pid is deleted` | read the jid *before* deleting the job |
| `All test case passed` | 16/16 |

## Job model

A fixed array of 16 job slots, each holding a PID, a job ID, a state, and the command line.
Three states:

```
        fork + addjob                    Ctrl-Z / SIGTSTP
  ----> FG (foreground) ---------------------------------> ST (stopped)
          |     ^                                            |    |
          |     | fg %n                              bg %n   |    | fg %n
          v     |                                            v    v
        reaped  +---------------------- BG (background) <----+----+
```

At most one job is FG at any moment; that invariant is what `waitfg` and the Ctrl-C/Ctrl-Z
handlers rely on to know who to signal.

## The fork/addjob race

The central problem of the lab. A short-lived child can be reaped by `sigchld_handler`
*before* the parent gets to `addjob`, so the handler tries to delete a job that isn't in the
list yet — and then the parent adds a job that will never be removed:

```
   parent                          child
     fork() -------------------->  exits immediately
     ...                           SIGCHLD delivered
        <-- handler runs: deletejob(pid)   ← job not in list yet
     addjob(pid)                          ← now it's there forever
```

The fix is to block `SIGCHLD` across the whole window, in
[`eval`](../../Labs/shlab-handout/shlab-handout/tsh.c#L167):

```c
sigprocmask(SIG_BLOCK, &mask, &prev);   /* block SIGCHLD */
pid = fork();
if (pid == 0) {
    sigprocmask(SIG_SETMASK, &prev, NULL);   /* child: unblock before exec */
    setpgid(0, 0);
    execv(argv[0], argv);
}
addjob(jobs, pid, bg ? BG : FG, cmdline);
sigprocmask(SIG_SETMASK, &prev, NULL);   /* parent: unblock after addjob */
```

The child restoring the mask matters as much as the parent blocking it: children inherit
the blocked set across `fork`, and `execv` preserves it, so without that line every program
the shell launches would start with `SIGCHLD` blocked — and would pass that on to *its*
children.

## Process groups

`setpgid(0, 0)` in the child puts each job in its own process group. Without it, jobs land
in the shell's group and a Ctrl-C from the terminal is delivered to the shell itself along
with every background job.

With separate groups, the handlers forward signals to the group with a negative PID:

```c
kill(-pid, SIGINT);    /* the whole group, not just the group leader */
```

That negation is what makes traces 11-13 pass: `mysplit` forks a child of its own, so the
job is two processes. Signalling `pid` would leave the grandchild running and orphaned;
signalling `-pid` takes down the group.

## waitfg without busy-waiting

The tempting implementations are a spin loop (burns a core) or `sleep(1)` in a loop (adds
latency, and still races). The correct one is
[`sigsuspend`](../../Labs/shlab-handout/shlab-handout/tsh.c#L378):

```c
while (pid == fgpid(jobs))
    sigsuspend(&empty);
```

`sigsuspend` atomically installs the mask and suspends, so there's no window between
"check the condition" and "go to sleep" in which the SIGCHLD could arrive and be missed.
The loop re-checks `fgpid` on wake because any signal can wake it, not just the one we care
about.

## Reaping

[`sigchld_handler`](../../Labs/shlab-handout/shlab-handout/tsh.c#L400) reaps in a loop,
because signals are not queued — several children exiting close together can produce a
single delivered SIGCHLD:

```c
while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) { ... }
```

`WNOHANG` keeps the handler from blocking on children that are still running; `WUNTRACED`
is what makes *stopped* children visible at all, so Ctrl-Z can be reported. The three
outcomes are distinguished by macro:

| macro | meaning | action |
|---|---|---|
| `WIFEXITED` | exited normally | `deletejob` |
| `WIFSIGNALED` | killed by a signal | report `WTERMSIG`, `deletejob` |
| `WIFSTOPPED` | stopped, still alive | report `WSTOPSIG`, mark `ST` — **no** delete |

The stopped case is the one that's easy to get wrong: the process still exists and must
stay in the job list so `bg`/`fg` can find it later.

Ordering bug worth recording (commit `Fixed converting pid2jid after pid is deleted`):
`pid2jid` walks the job list, so it must be called *before* `deletejob`. Reversed, it looks
up a slot that has already been cleared and prints job 0.

## Signal-handler discipline

Every handler follows the same three rules:

```c
int olderrno = errno;                        /* 1. save errno */
sigfillset(&allmask);
sigprocmask(SIG_SETMASK, &allmask, &prev);   /* 2. block everything */
    ...
sigprocmask(SIG_SETMASK, &prev, NULL);
errno = olderrno;                            /* 3. restore errno */
```

Saving `errno` matters because a handler can interrupt main-line code between a failed
syscall and its `errno` check; `waitpid` returning `ECHILD` would otherwise overwrite the
value the interrupted code was about to read. Blocking all signals while touching the job
list keeps a second handler from seeing it half-updated.

## Limitations

- **`fprintf` in signal handlers is not async-signal-safe.** Strictly it should be a
  `write()`-based routine on a hand-formatted buffer. It works here because the shell never
  calls `printf` concurrently with a handler in a way that corrupts stdio state, but it is
  the one place the implementation trades correctness-by-the-book for readability.
- **`WIFSTOPPED` only recognises `SIGTSTP`.** A child stopped by `SIGSTOP` is reaped from
  `waitpid` but its state isn't updated. Trace 16 uses `SIGTSTP`, so it passes.
- **A background job stopped externally keeps state `BG`.** The state update is guarded by
  `current->state == FG`, so `jobs` would report it as Running.
- No pipes, no redirection, no globbing, no variable expansion. Quoting is limited to
  single quotes, from the handout's `parseline`.
- Job slots are a fixed array of 16 with no growth.

## Build

```sh
make
./tsh                                        # interactive
./sdriver.pl -t trace07.txt -s ./tsh -a "-p" # one trace
make test07                                  # same, via the Makefile
```

`eval`, `builtin_cmd`, `do_bgfg`, `waitfg`, and the three signal handlers are mine. The job
list helpers, `parseline`, `main`, the trace files, `sdriver.pl`, and the `myspin`/`mysplit`/
`mystop`/`myint` test programs are CS:APP course materials.
