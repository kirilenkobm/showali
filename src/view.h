#pragma once
#include "parser_fasta.h"

typedef struct {
    SeqList *seqs;      // all sequences
    int      row_offset; // index of the first sequence row shown
    int      col_offset; // index of the first base shown
    int      rows;       // terminal height
    int      cols;       // terminal width
} ViewState;

// initialize view state from parsed sequences
ViewState view_init(SeqList *s);

// adjust to a terminal resize
void view_resize(ViewState *vs);

// vertical scrolling
void view_scroll_up(ViewState *vs);
void view_scroll_down(ViewState *vs);

// horizontal scrolling
void view_scroll_left(ViewState *vs);
void view_scroll_right(ViewState *vs);