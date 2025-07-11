#pragma once
#include <stddef.h>

typedef enum {
    SEQ_DNA,
    SEQ_RNA,
    SEQ_PROTEIN,
    SEQ_UNKNOWN
} SequenceType;

typedef struct {
    char *id;
    char *seq;
    size_t len;
    SequenceType type;
} Sequence;

typedef struct {
    Sequence *items;
    size_t count, capacity;
} SeqList;

SeqList *parse_fasta(const char *path);
