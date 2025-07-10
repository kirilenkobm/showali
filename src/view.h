#pragma once
#include "parser_fasta.h"

typedef struct {
    SeqList *seqs;
    int      row_offset;
    int      col_offset;
    int      rows, cols;
} ViewState;

ViewState view_init(SeqList *s);
void      view_resize(ViewState *vs);
void      view_scroll_up(ViewState *vs);
void      view_scroll_down(ViewState *vs);
void      view_scroll_left(ViewState *vs);
void      view_scroll_right(ViewState *vs);
