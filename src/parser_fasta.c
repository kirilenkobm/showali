#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser_fasta.h"

// simple growable arrays
#define MAX_SEQ 1000000

SeqList *parse_fasta(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror("fopen"); exit(1); }

    SeqList *sl = calloc(1, sizeof(*sl));
    sl->capacity = 16;
    sl->items    = calloc(sl->capacity, sizeof(Sequence));

    char *line = NULL;
    size_t  len = 0;
    Sequence *cur = NULL;

    while (getline(&line, &len, f) != -1) {
        if (line[0] == '>') {
            if (sl->count == sl->capacity) {
                sl->capacity *= 2;
                sl->items = realloc(sl->items, sl->capacity * sizeof(Sequence));
            }
            cur = &sl->items[sl->count++];
            cur->id = strdup(line + 1);
            cur->seq = calloc(1, MAX_SEQ);
            cur->len = 0;
        } else if (cur) {
            // strip newline & append
            char *p = strchr(line, '\n');
            if (p) *p = 0;
            size_t l = strlen(line);
            memcpy(cur->seq + cur->len, line, l);
            cur->len += l;
        }
    }
    free(line);
    fclose(f);
    return sl;
}
