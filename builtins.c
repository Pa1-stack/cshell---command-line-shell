#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "shell.h"

static const char *BUILTINS[] = {
    "cd", "pwd", "exit", "jobs", "fg", "bg", "export", NULL
};

int is_builtin(const char *cmd) {
    for (int i = 0; BUILTINS[i]; i++)
        if (strcmp(cmd, BUILTINS[i]) == 0) return 1;
    return 0;
}

/* Builtins run in the shell's own process — no fork — because their
   whole point (cd, exit, export) is to mutate the shell's own state.
   Forking would mutate a child's state and vanish on exit(). */
int run_builtin(char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        const char *path = argv[1] ? argv[1] : getenv("HOME");
        if (chdir(path) != 0) perror("cd");
        return 1;
    }
    if (strcmp(argv[0], "pwd") == 0) {
        char buf[1024];
        if (getcwd(buf, sizeof(buf))) printf("%s\n", buf);
        return 1;
    }
    if (strcmp(argv[0], "exit") == 0) {
        exit(argv[1] ? atoi(argv[1]) : 0);
    }
    if (strcmp(argv[0], "jobs") == 0) {
        job_list_print();
        return 1;
    }
    if (strcmp(argv[0], "export") == 0) {
        if (argv[1]) {
            char *eq = strchr(argv[1], '=');
            if (eq) {
                *eq = '\0';
                setenv(argv[1], eq + 1, 1);
            }
        }
        return 1;
    }
    if (strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0) {
        pid_t pgid = argv[1] ? atoi(argv[1]) : 0;
        Job *j = job_find(pgid);
        if (!j) { fprintf(stderr, "%s: no such job\n", argv[0]); return 1; }
        kill(-j->pgid, SIGCONT);
        j->state = JOB_RUNNING;
        if (strcmp(argv[0], "fg") == 0) {
            tcsetpgrp(shell_terminal, j->pgid);
            int status;
            waitpid(-j->pgid, &status, WUNTRACED);
            tcsetpgrp(shell_terminal, shell_pgid);
            if (WIFSTOPPED(status)) j->state = JOB_STOPPED;
            else job_remove(j->pgid);
        } else {
            printf("[%d] %d continued in background\n", j->id, j->pgid);
        }
        return 1;
    }
    return 0;
}
