#pragma once
#include "parser_fasta.h"
#include <stdbool.h>

typedef struct {
    SeqList *seqs;      // all sequences
    int      row_offset; // index of the first sequence row shown
    int      col_offset; // index of the first base shown
    int      rows;       // terminal height
    int      cols;       // terminal width
    bool     jump_mode;  // true when collecting jump position
    char     jump_buffer[16]; // buffer for collecting jump digits
    int      jump_pos;   // current position in jump buffer
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

// jump mode handling
void view_start_jump(ViewState *vs);
void view_add_jump_digit(ViewState *vs, char digit);
void view_execute_jump(ViewState *vs);
void view_cancel_jump(ViewState *vs);