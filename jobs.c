#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "shell.h"

static Job *job_head = NULL;
static int next_id = 1;

void jobs_init(void) { job_head = NULL; }

Job *job_add(pid_t pgid, const char *cmdline, JobState state) {
    Job *j = malloc(sizeof(Job));
    j->id = next_id++;
    j->pgid = pgid;
    strncpy(j->cmdline, cmdline, MAX_LINE - 1);
    j->cmdline[MAX_LINE - 1] = '\0';
    j->state = state;
    j->next = job_head;
    job_head = j;
    return j;
}

Job *job_find(pid_t pgid) {
    /* fg/bg accept either an explicit pgid or "most recent" (0) */
    if (pgid == 0) return job_head;
    for (Job *j = job_head; j; j = j->next)
        if (j->pgid == pgid || j->id == pgid) return j;
    return NULL;
}

void job_remove(pid_t pgid) {
    Job **cur = &job_head;
    while (*cur) {
        if ((*cur)->pgid == pgid) {
            Job *dead = *cur;
            *cur = dead->next;
            free(dead);
            return;
        }
        cur = &(*cur)->next;
    }
}

void job_list_print(void) {
    for (Job *j = job_head; j; j = j->next) {
        const char *state = j->state == JOB_RUNNING ? "Running" :
                             j->state == JOB_STOPPED ? "Stopped" : "Done";
        printf("[%d] %d %-8s %s\n", j->id, j->pgid, state, j->cmdline);
    }
}

/* Reaps every terminated/stopped child without blocking, using
   WNOHANG + WUNTRACED so the shell prompt is never held up waiting
   on background jobs. This is what makes '&' actually asynchronous. */
void job_reap(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        for (Job *j = job_head; j; j = j->next) {
            if (j->pgid == pid) {
                if (WIFSTOPPED(status)) j->state = JOB_STOPPED;
                else if (WIFCONTINUED(status)) j->state = JOB_RUNNING;
                else { j->state = JOB_DONE; }
            }
        }
    }
}

/* Registered for SIGCHLD. Kept minimal (async-signal-safe subset)
   — it just triggers reaping via the same waitpid loop; printing job
   status happens later in the prompt loop, not inside the handler. */
void sigchld_handler(int sig) {
    (void)sig;
    job_reap();
}
