#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "shell.h"

/* Shell must put itself in its own process group and, if interactive,
   take control of the terminal (tcsetpgrp) and ignore job-control
   signals so Ctrl-C typed at the prompt doesn't kill the shell itself
   — this is the standard "become a job-control shell" dance from
   Ā glibc's job-control manual. */
static void shell_init(void) {
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        signal(SIGINT,  SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, sigchld_handler);

        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }
        tcsetpgrp(shell_terminal, shell_pgid);
    }
    jobs_init();
}

static void print_prompt(void) {
    char cwd[512];
    getcwd(cwd, sizeof(cwd));
    printf("cshell:%s$ ", cwd);
    fflush(stdout);
}

int main(void) {
    shell_init();
    char line[MAX_LINE];

    while (1) {
        job_reap();          /* pick up finished background jobs */
        print_prompt();

        if (!fgets(line, sizeof(line), stdin)) {
            if (feof(stdin)) { printf("\n"); break; } /* Ctrl-D */
            continue;                                  /* interrupted read */
        }
        if (line[0] == '\n') continue;

        char raw_copy[MAX_LINE];
        strncpy(raw_copy, line, MAX_LINE - 1);
        raw_copy[MAX_LINE - 1] = '\0';

        Pipeline pl;
        if (parse_line(line, &pl) != 0) {
            fprintf(stderr, "cshell: parse error\n");
            continue;
        }
        run_pipeline(&pl, raw_copy);
    }
    return 0;
}
