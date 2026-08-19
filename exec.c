#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "shell.h"

pid_t shell_pgid;
int shell_terminal;
int shell_is_interactive;

static void launch_child(Command *c, int infd, int outfd, pid_t pgid,
                          int background) {
    /* Each child: (1) joins the pipeline's process group, (2) restores
       default signal disposition (shell ignores SIGINT/SIGTSTP so it
       survives Ctrl-C; children must NOT inherit that), (3) wires its
       stdin/stdout to the pipe or file, (4) execs. */
    pid_t pid = getpid();
    if (pgid == 0) pgid = pid;
    setpgid(pid, pgid);
    if (!background) tcsetpgrp(shell_terminal, pgid);

    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    if (infd != STDIN_FILENO) { dup2(infd, STDIN_FILENO); close(infd); }
    if (outfd != STDOUT_FILENO) { dup2(outfd, STDOUT_FILENO); close(outfd); }

    if (c->infile) {
        int fd = open(c->infile, O_RDONLY);
        if (fd < 0) { perror(c->infile); _exit(1); }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (c->outfile) {
        int flags = O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC);
        int fd = open(c->outfile, flags, 0644);
        if (fd < 0) { perror(c->outfile); _exit(1); }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    execvp(c->argv[0], c->argv);
    fprintf(stderr, "%s: command not found\n", c->argv[0]);
    _exit(127);
}

void run_pipeline(Pipeline *pl, char *raw_line) {
    /* Single builtin, no pipe, foreground: run in-process, no fork. */
    if (pl->ncmds == 1 && !pl->background && is_builtin(pl->cmds[0].argv[0])) {
        run_builtin(pl->cmds[0].argv);
        return;
    }

    int nprocs = pl->ncmds;
    pid_t pids[MAX_CMDS];
    pid_t pgid = 0;
    int infd = STDIN_FILENO;
    int pipefd[2];

    for (int i = 0; i < nprocs; i++) {
        int outfd = STDOUT_FILENO;
        int have_pipe = (i < nprocs - 1);
        if (have_pipe) {
            if (pipe(pipefd) < 0) { perror("pipe"); return; }
            outfd = pipefd[1];
        }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return; }

        if (pid == 0) {
            /* child: only needs its own end of the current pipe */
            if (have_pipe) close(pipefd[0]);
            launch_child(&pl->cmds[i], infd, outfd, pgid, pl->background);
        }

        /* parent: track pgid, close fds it no longer needs, chain infd */
        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid); /* set on both ends — avoids a fork/exec race */
        pids[i] = pid;

        if (infd != STDIN_FILENO) close(infd);
        if (have_pipe) { close(pipefd[1]); infd = pipefd[0]; }
    }

    char cmdline_copy[MAX_LINE];
    strncpy(cmdline_copy, raw_line, MAX_LINE - 1);
    cmdline_copy[MAX_LINE - 1] = '\0';

    if (pl->background) {
        job_add(pgid, cmdline_copy, JOB_RUNNING);
        printf("[bg] pgid %d\n", pgid);
        return; /* don't wait — shell keeps prompting */
    }

    /* Foreground: hand the controlling terminal to the pipeline's
       group, block until it's done or stopped, then reclaim it.
       This terminal handoff is *the* mechanism that makes Ctrl-C /
       Ctrl-Z affect the running program instead of the shell. */
    int status;
    int last_status = 0;
    for (int i = 0; i < nprocs; i++) {
        waitpid(pids[i], &status, WUNTRACED);
        if (WIFSTOPPED(status)) {
            job_add(pgid, cmdline_copy, JOB_STOPPED);
            printf("\n[Stopped] %s\n", cmdline_copy);
        }
        last_status = status;
    }
    (void)last_status;
    tcsetpgrp(shell_terminal, shell_pgid);
}
