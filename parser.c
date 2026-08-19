#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"

/* Splits on '|', then within each segment pulls out <, >, >> tokens
   and builds argv. Whitespace-delimited only (no quoting) — kept
   simple on purpose so the parser's control flow stays inspectable. */

static char *dup_trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t')) *end-- = '\0';
    return strdup(s);
}

int parse_line(char *line, Pipeline *pl) {
    memset(pl, 0, sizeof(*pl));

    /* strip trailing newline and detect background '&' */
    char *nl = strchr(line, '\n');
    if (nl) *nl = '\0';

    char *amp = strrchr(line, '&');
    if (amp && *(amp + 1) == '\0') {
        pl->background = 1;
        *amp = '\0';
    }

    /* split on pipes */
    char *segments[MAX_CMDS];
    int nseg = 0;
    char *saveptr1;
    char *tok = strtok_r(line, "|", &saveptr1);
    while (tok && nseg < MAX_CMDS) {
        segments[nseg++] = tok;
        tok = strtok_r(NULL, "|", &saveptr1);
    }
    if (nseg == 0) return -1;
    pl->ncmds = nseg;

    for (int i = 0; i < nseg; i++) {
        Command *c = &pl->cmds[i];
        char *seg = dup_trim(segments[i]);
        char *saveptr2;
        char *word = strtok_r(seg, " \t", &saveptr2);
        int argc = 0;

        while (word) {
            if (strcmp(word, "<") == 0) {
                word = strtok_r(NULL, " \t", &saveptr2);
                if (word) c->infile = strdup(word);
            } else if (strcmp(word, ">") == 0) {
                word = strtok_r(NULL, " \t", &saveptr2);
                if (word) { c->outfile = strdup(word); c->append = 0; }
            } else if (strcmp(word, ">>") == 0) {
                word = strtok_r(NULL, " \t", &saveptr2);
                if (word) { c->outfile = strdup(word); c->append = 1; }
            } else {
                if (argc < MAX_ARGS - 1) c->argv[argc++] = strdup(word);
            }
            word = strtok_r(NULL, " \t", &saveptr2);
        }
        c->argv[argc] = NULL;
        free(seg);
        if (argc == 0) return -1; /* empty segment, e.g. "ls ||" */
    }
    return 0;
}
