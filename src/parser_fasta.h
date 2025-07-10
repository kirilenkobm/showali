#pragma once
#include <stddef.h>
typedef struct {
    char *id;
    char *seq;
    size_t len;
} Sequence;
typedef struct {
    Sequence *items;
    size_t count, capacity;
} SeqList;
SeqList *parse_fasta(const char *path);
