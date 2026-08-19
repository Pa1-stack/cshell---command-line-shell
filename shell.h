#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>

#define MAX_ARGS 64
#define MAX_CMDS 16     /* max commands chained by pipes */
#define MAX_LINE 1024

typedef struct {
    char *argv[MAX_ARGS];   /* NULL-terminated argv for execvp */
    char *infile;           /* redirection target, NULL if none */
    char *outfile;
    int   append;           /* 1 => >>, 0 => > */
} Command;

typedef struct {
    Command cmds[MAX_CMDS];
    int ncmds;
    int background;         /* trailing & */
} Pipeline;

typedef enum { JOB_RUNNING, JOB_STOPPED, JOB_DONE } JobState;

typedef struct Job {
    int id;
    pid_t pgid;
    char cmdline[MAX_LINE];
    JobState state;
    struct Job *next;
} Job;

/* parser.c */
int parse_line(char *line, Pipeline *pl);

/* jobs.c */
void jobs_init(void);
Job *job_add(pid_t pgid, const char *cmdline, JobState state);
void job_remove(pid_t pgid);
Job *job_find(pid_t pgid);
void job_reap(void);            /* called via SIGCHLD / after fg wait */
void job_list_print(void);
void sigchld_handler(int sig);

/* exec.c */
void run_pipeline(Pipeline *pl, char *raw_line);

/* builtins.c */
int is_builtin(const char *cmd);
int run_builtin(char **argv);

extern pid_t shell_pgid;
extern int shell_terminal;
extern int shell_is_interactive;

#endif
